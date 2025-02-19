// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/safe_browsing_navigation_observer.h"

#include "base/memory/ptr_util.h"
#include "base/time/time.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/safe_browsing/safe_browsing_navigation_observer_manager.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/resource_request_details.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/resource_type.h"

using content::WebContents;

namespace {
const char kWebContentsUserDataKey[] =
    "web_contents_safe_browsing_navigation_observer";
}  // namespace

namespace safe_browsing {

// SafeBrowsingNavigationObserver::NavigationEvent-----------------------------
NavigationEvent::NavigationEvent()
    : source_url(),
      source_main_frame_url(),
      original_request_url(),
      destination_url(),
      source_tab_id(-1),
      target_tab_id(-1),
      frame_id(-1),
      last_updated(base::Time::Now()),
      is_user_initiated(false),
      has_committed(false),
      has_server_redirect(false) {}

NavigationEvent::NavigationEvent(NavigationEvent&& nav_event)
    : source_url(std::move(nav_event.source_url)),
      source_main_frame_url(std::move(nav_event.source_main_frame_url)),
      original_request_url(std::move(nav_event.original_request_url)),
      destination_url(std::move(nav_event.destination_url)),
      source_tab_id(std::move(nav_event.source_tab_id)),
      target_tab_id(std::move(nav_event.target_tab_id)),
      frame_id(nav_event.frame_id),
      last_updated(nav_event.last_updated),
      is_user_initiated(nav_event.is_user_initiated),
      has_committed(nav_event.has_committed),
      has_server_redirect(nav_event.has_server_redirect) {}

NavigationEvent& NavigationEvent::operator=(NavigationEvent&& nav_event) {
  source_url = std::move(nav_event.source_url);
  source_main_frame_url = std::move(nav_event.source_main_frame_url);
  original_request_url = std::move(nav_event.original_request_url);
  destination_url = std::move(nav_event.destination_url);
  source_tab_id = nav_event.source_tab_id;
  target_tab_id = nav_event.target_tab_id;
  frame_id = nav_event.frame_id;
  last_updated = nav_event.last_updated;
  is_user_initiated = nav_event.is_user_initiated;
  has_committed = nav_event.has_committed;
  has_server_redirect = nav_event.has_server_redirect;
  return *this;
}

NavigationEvent::~NavigationEvent() {}

// SafeBrowsingNavigationObserver --------------------------------------------

// static
void SafeBrowsingNavigationObserver::MaybeCreateForWebContents(
    content::WebContents* web_contents) {
  if (FromWebContents(web_contents))
    return;

  if (safe_browsing::SafeBrowsingNavigationObserverManager::IsEnabledAndReady(
        Profile::FromBrowserContext(web_contents->GetBrowserContext()))) {
    web_contents->SetUserData(
        kWebContentsUserDataKey,
        new SafeBrowsingNavigationObserver(
            web_contents,
            g_browser_process->safe_browsing_service()
                ->navigation_observer_manager()));
  }
}

// static
SafeBrowsingNavigationObserver* SafeBrowsingNavigationObserver::FromWebContents(
    content::WebContents* web_contents) {
  return static_cast<SafeBrowsingNavigationObserver*>(
      web_contents->GetUserData(kWebContentsUserDataKey));
}

SafeBrowsingNavigationObserver::SafeBrowsingNavigationObserver(
    content::WebContents* contents,
    const scoped_refptr<SafeBrowsingNavigationObserverManager>& manager)
    : content::WebContentsObserver(contents),
      manager_(manager),
      has_user_gesture_(false),
      last_user_gesture_timestamp_(base::Time()) {}

SafeBrowsingNavigationObserver::~SafeBrowsingNavigationObserver() {}

// Called when a navigation starts in the WebContents. |navigation_handle|
// parameter is unique to this navigation, which will appear in the following
// DidRedirectNavigation, and DidFinishNavigation too.
void SafeBrowsingNavigationObserver::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  NavigationEvent nav_event;
  auto it = navigation_handle_map_.find(navigation_handle);
  // It is possible to see multiple DidStartNavigation(..) with the same
  // navigation_handle (e.g. cross-process transfer). If that's the case,
  // we need to copy the is_user_initiated field.
  if (it != navigation_handle_map_.end()) {
    nav_event.is_user_initiated = it->second.is_user_initiated;
  } else {
    // If this is the first time we see this navigation_handle, create a new
    // NavigationEvent, and decide if it is triggered by user.
    if ((has_user_gesture_ &&
         !SafeBrowsingNavigationObserverManager::IsUserGestureExpired(
             last_user_gesture_timestamp_)) ||
        !navigation_handle->IsRendererInitiated()) {
      nav_event.is_user_initiated = true;
      if (has_user_gesture_) {
        manager_->OnUserGestureConsumed(web_contents(),
                                        last_user_gesture_timestamp_);
        has_user_gesture_ = false;
      }
    }
  }

  // All the other fields are reconstructed based on current content of
  // navigation_handle.
  nav_event.frame_id = navigation_handle->GetFrameTreeNodeId();

  // If there was a URL previously committed in the current RenderFrameHost,
  // set it as the source url of this navigation. Otherwise, this is the
  // first url going to commit in this frame. We set navigation_handle's URL as
  // the source url.
  // TODO(jialiul): source_url, source_tab_id, and source_main_frame_url may be
  // incorrect when another frame is targeting this frame. Need to refine this
  // logic after the true initiator details are added to NavigationHandle
  // (https://crbug.com/651895).
  content::RenderFrameHost* current_frame_host =
      navigation_handle->GetWebContents()->FindFrameByFrameTreeNodeId(
          nav_event.frame_id);
  // For browser initiated navigation (e.g. from address bar or bookmark), we
  // don't fill the source_url to prevent attributing navigation to the last
  // committed navigation.
  if (navigation_handle->IsRendererInitiated() && current_frame_host &&
      current_frame_host->GetLastCommittedURL().is_valid()) {
    nav_event.source_url = SafeBrowsingNavigationObserverManager::ClearEmptyRef(
        current_frame_host->GetLastCommittedURL());
  }
  nav_event.original_request_url =
      SafeBrowsingNavigationObserverManager::ClearEmptyRef(
          navigation_handle->GetURL());
  nav_event.destination_url = nav_event.original_request_url;

  nav_event.source_tab_id =
      SessionTabHelper::IdForTab(navigation_handle->GetWebContents());

  if (navigation_handle->IsInMainFrame()) {
    nav_event.source_main_frame_url = nav_event.source_url;
  } else {
    nav_event.source_main_frame_url =
        SafeBrowsingNavigationObserverManager::ClearEmptyRef(
            navigation_handle->GetWebContents()->GetLastCommittedURL());
  }
  navigation_handle_map_[navigation_handle] = std::move(nav_event);
}

void SafeBrowsingNavigationObserver::DidRedirectNavigation(
    content::NavigationHandle* navigation_handle) {
  // We should have already seen this navigation_handle in DidStartNavigation.
  if (navigation_handle_map_.find(navigation_handle) ==
      navigation_handle_map_.end()) {
    NOTREACHED();
    return;
  }

  NavigationEvent* nav_event = &navigation_handle_map_[navigation_handle];
  nav_event->has_server_redirect = true;
  nav_event->destination_url =
      SafeBrowsingNavigationObserverManager::ClearEmptyRef(
          navigation_handle->GetURL());
  nav_event->last_updated = base::Time::Now();
}

void SafeBrowsingNavigationObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle_map_.find(navigation_handle) ==
      navigation_handle_map_.end()) {
    NOTREACHED();
    return;
  }

  // If it is an error page, we ignore this navigation.
  if (navigation_handle->IsErrorPage()) {
    navigation_handle_map_.erase(navigation_handle);
    return;
  }
  NavigationEvent* nav_event = &navigation_handle_map_[navigation_handle];

  nav_event->has_committed = navigation_handle->HasCommitted();
  nav_event->target_tab_id =
      SessionTabHelper::IdForTab(navigation_handle->GetWebContents());
  nav_event->last_updated = base::Time::Now();

  manager_->RecordNavigationEvent(nav_event->destination_url, nav_event);
  navigation_handle_map_.erase(navigation_handle);
}

void SafeBrowsingNavigationObserver::DidGetResourceResponseStart(
    const content::ResourceRequestDetails& details) {
  // We only care about main frame and sub frame.
  if (details.resource_type != content::RESOURCE_TYPE_MAIN_FRAME &&
      details.resource_type != content::RESOURCE_TYPE_SUB_FRAME) {
    return;
  }
  if (!details.url.is_valid() || details.socket_address.IsEmpty())
    return;
  if (!details.url.host().empty()) {
    manager_->RecordHostToIpMapping(details.url.host(),
                                    details.socket_address.host());
  }
}

void SafeBrowsingNavigationObserver::DidGetUserInteraction(
    const blink::WebInputEvent::Type type) {
  last_user_gesture_timestamp_ = base::Time::Now();
  has_user_gesture_ = true;
  manager_->RecordUserGestureForWebContents(web_contents(),
                                            last_user_gesture_timestamp_);
}

void SafeBrowsingNavigationObserver::WebContentsDestroyed() {
  manager_->OnWebContentDestroyed(web_contents());
  web_contents()->RemoveUserData(kWebContentsUserDataKey);
  // web_contents is null after this function.
}

}  // namespace safe_browsing
