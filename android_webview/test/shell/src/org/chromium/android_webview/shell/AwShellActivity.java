// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.shell;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.res.AssetFileDescriptor;
import android.os.Bundle;
import android.os.Debug;
import android.text.TextUtils;
import android.text.InputType;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnFocusChangeListener;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.webkit.GeolocationPermissions;
import android.webkit.WebChromeClient;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;

import org.chromium.android_webview.AwBrowserContext;
import org.chromium.android_webview.AwBrowserProcess;
import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.AwContentsClient;
import org.chromium.android_webview.AwWebResourceResponse;
import org.chromium.android_webview.AwContentsClient.AwWebResourceRequest;
import org.chromium.android_webview.AwContentsStatics;
import org.chromium.android_webview.AwDevToolsServer;
import org.chromium.android_webview.AwSettings;
import org.chromium.android_webview.test.AwTestContainerView;
import org.chromium.android_webview.test.NullContentsClient;
import org.chromium.android_webview.JsResultReceiver;
import org.chromium.android_webview.JsPromptResultReceiver;
import org.chromium.base.BaseSwitches;
import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.TraceEvent;
import org.chromium.content.app.ContentApplication;
import org.chromium.content_public.browser.NavigationController;
import org.chromium.content_public.browser.WebContents;

import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.net.URI;
import java.net.URISyntaxException;
import android.app.AlertDialog;
import android.content.DialogInterface;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import android.widget.FrameLayout;
import java.text.DecimalFormat;

import java.util.ArrayList;

import org.json.JSONException;
import org.json.JSONObject;

import android.media.AudioManager;
import android.media.MediaPlayer;
import android.graphics.SurfaceTexture;

/**
 * This is a lightweight activity for tests that only require WebView functionality.
 */
public class AwShellActivity extends Activity implements 
        SurfaceTexture.OnFrameAvailableListener,
        MediaPlayer.OnVideoSizeChangedListener,
        MediaPlayer.OnCompletionListener,
        MediaPlayer.OnErrorListener,
        MediaPlayer.OnPreparedListener,
        AudioManager.OnAudioFocusChangeListener,
        SurfaceHolder.Callback {  
    private static final String TAG = "cr.AwShellActivity";
    private static final String PREFERENCES_NAME = "AwShellPrefs";
    private static final String INITIAL_URL = "No URL provided either in the intent nor in the config.json";
    private static final String LAST_USED_URL_KEY = "url";
    private AwBrowserContext mBrowserContext;
    private AwDevToolsServer mDevToolsServer;
    private AwTestContainerView mAwTestContainerView;

    private SurfaceView surfaceView;
    private SurfaceHolder surfaceHolder;
    private long nativePointer = 0;   

    private class Video
    {
        public SurfaceTexture surfaceTexture = null;
        public Surface surface = null;
        public MediaPlayer mediaPlayer = null;
        public int nativeTextureId = -1;
    }

    ArrayList<Video> videos = new ArrayList<Video>();
    private AudioManager audioManager = null;

    private String vrWebGLJSCode; 
    private String url;
    private boolean urlLoaded = false;

    private JSONObject config = null;

    private FrameLayout layout;

    // This is the same as data_reduction_proxy::switches::kEnableDataReductionProxy.
    private static final String ENABLE_DATA_REDUCTION_PROXY = "enable-spdy-proxy-auth";
    // This is the same as data_reduction_proxy::switches::kDataReductionProxyKey.
    private static final String DATA_REDUCTION_PROXY_KEY = "spdy-proxy-auth-value";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        CommandLine.init(new String[] { "chrome", "--ignore-gpu-blacklist", "--enable-webvr" });

        AwShellResourceProvider.registerResources(this);

        ContentApplication.initCommandLine(this);
        waitForDebuggerIfNeeded();

        ContextUtils.initApplicationContext(getApplicationContext());
        AwBrowserProcess.loadLibrary();
        System.loadLibrary("VRWebGL_OculusMobileSDK");

        if (CommandLine.getInstance().hasSwitch(AwShellSwitches.ENABLE_ATRACE)) {
            Log.e(TAG, "Enabling Android trace.");
            TraceEvent.setATraceEnabled(true);
        }

        layout = new FrameLayout(this); 

        setContentView(layout);       

        mAwTestContainerView = createAwTestContainerView();

        layout.addView(mAwTestContainerView);           

        surfaceView = new SurfaceView( this );
        layout.addView(surfaceView);
        surfaceView.setZOrderMediaOverlay(true);
        surfaceView.getHolder().addCallback( this );   

        // Load the VRWebGL.js file
        try
        {
            vrWebGLJSCode = Utils.readFromAssets(this, "VRWebGL.js");
        }
        catch(IOException e)
        {
            System.err.println("VRWebGL: IOException while reading the VRWebGL.js file.");
            e.printStackTrace();
            Utils.createAlertDialog(this, "Error loading extension file", "IOException while reading the extension file. Load the page anyway?", new DialogInterface.OnClickListener()
            {
                @Override
                public void onClick(DialogInterface dialog, int which)
                {
                    if (which == DialogInterface.BUTTON_NEGATIVE)
                    {
                        finish();
                    }
                }
            }, 2, "Yes", "No", null);
        }

        // Load the config.json file
        String configString = null;
        try
        {
            configString = Utils.readFromAssets(this, "config.json");
            try
            {
                // If the file was read, try to create the corresponding JSON object
                config = new JSONObject(configString);
                System.out.println("VRWebGL: config created from config.json asset file.");
            }
            catch(JSONException e) 
            {
                System.out.println("VRWebGL: WARNING: JSONException while parsing the config.json content: '" + configString + "': " + e.getCause() + " - " + e.getMessage());
            }
        }
        catch(IOException e)
        {
            System.out.println("VRWebGL: WARNING: IOException while loading the 'config.json' asset file: " + e.getCause() + " - " + e.getMessage());
        }
        // The intent, when provided, has more priority than the config asset file.
        configString = getConfigStringFromIntent(getIntent());
        if (configString != null) 
        {
            try
            {
                config = new JSONObject(configString);
                System.out.println("VRWebGL: config created from intent.");
            }
            catch(JSONException e)
            {
                System.out.println("VRWebGL: WARNING: JSONException while parsing the intent config content: '" + configString + "': " + e.getCause() + " - " + e.getMessage());
            }
        }
        // If the configuration was provided, use it.
        boolean clearCache = true;
        if (config != null) 
        {
            try
            {
                url = config.getString("url");
                clearCache = config.has("clearCache") ? config.getBoolean("clearCache") : true;
            }
            catch(JSONException e) 
            {
                System.out.println("VRWebGL: WARNING: JSONException while trying to retrieve the config properties: " + e.getCause() + " - " + e.getMessage());
            }
        }
        mAwTestContainerView.getAwContents().clearCache(clearCache);
        System.out.println("VRWebGL: clearCache = " + clearCache);

        // The intent, when provided, has more priority than the config asset file.
        String intentURL = getUrlFromIntent(getIntent());
        if (TextUtils.isEmpty(intentURL)) {
            if (TextUtils.isEmpty(url))
            {
                url = INITIAL_URL;
            }
        }
        else 
        {
            url = intentURL;
        }

        AwContents.setShouldDownloadFavicons();

        audioManager = (AudioManager) getSystemService( Context.AUDIO_SERVICE );

        // Create the native side
        nativePointer = nativeOnCreate( this );        
    }

    @Override protected void onStart()
    {
        super.onStart();
        nativeOnStart( nativePointer );
    }

    @Override protected void onResume()
    {
        super.onResume();
        nativeOnResume( nativePointer );
    }

    @Override protected void onPause()
    {
        nativeOnPause( nativePointer );
        super.onPause();
    }

    @Override protected void onStop()
    {
        nativeOnStop( nativePointer );
        super.onStop();
    }

    @Override
    public void onDestroy() {
        if (mDevToolsServer != null) {
            mDevToolsServer.destroy();
            mDevToolsServer = null;
        }

        if ( surfaceHolder != null )
        {
            nativeOnSurfaceDestroyed( nativePointer );
        }

        nativeOnDestroy( nativePointer );
        super.onDestroy();
        nativePointer = 0;

        System.out.println("VRWebGL: onDestroy 4");
    }

    private AwTestContainerView createAwTestContainerView() {
        AwBrowserProcess.start();
        AwTestContainerView testContainerView = new AwTestContainerView(this, true);
        AwContentsClient awContentsClient = new NullContentsClient() {
            private View mCustomView;
            private boolean jsInjected = false;

            @Override
            public void onPageStarted(String url) {
                System.out.println("VRWebGL: onPageStarted url = " + url);
                // Reset 
                jsInjected = false;
                nativeOnPageStarted();
            }

            @Override
            public void onPageFinished(String url) {
                System.out.println("VRWebGL: onPageFinished url = " + url);
            }

            @Override
            public void onLoadResource(String url) {
                System.out.println("VRWebGL: onLoadResource url = " + url);
            }

            @Override
            public AwWebResourceResponse shouldInterceptRequest(
                    AwContentsClient.AwWebResourceRequest request) {
                System.out.println("VRWebGL: shouldInterceptRequest. request.url = " + request.url);        
                // TODO: This is a dirty hack. As it seems that the evalueJavaScript when the main page is loaded does not work because
                // the injected JS is wiped out, we will inject it when another resource is requested. This may or may not work in many 
                // cases depending on when the resource is requested. 
                // Waiting for a better answer from the chromium forums.
                if (!request.url.equals(AwShellActivity.this.url) && !jsInjected)
                {
                    mAwTestContainerView.getAwContents().evaluateJavaScript(vrWebGLJSCode, null);
                    jsInjected = true;
                }
                return null;
            }

            @Override
            public void onDownloadStart(String url,
                                        String userAgent,
                                        String contentDisposition,
                                        String mimeType,
                                        long contentLength) {
                System.out.println("VRWebGL: onDownloadStart. url = " + url);
            }

            @Override
            public void handleJsAlert(String url, String message, final JsResultReceiver receiver) {
                Utils.createAlertDialog(AwShellActivity.this, url, message, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        receiver.confirm();
                    }
                }, 1, "Ok", null, null).show();                
            }

            @Override
            public void handleJsPrompt(String url, String message, String defaultValue, final JsPromptResultReceiver receiver) {
                final EditText editText = new EditText(AwShellActivity.this);
                Utils.createPromptDialog(AwShellActivity.this, editText, url, message, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        if (which == DialogInterface.BUTTON_POSITIVE)
                        {
                            receiver.confirm(editText.getText().toString());
                        }
                        else 
                        {
                            receiver.cancel();
                        }
                    }
                }, 2, "Ok", "Cancel", null).show();                
            }

            @Override
            public void handleJsConfirm(String url, String message, final JsResultReceiver receiver) {
                Utils.createAlertDialog(AwShellActivity.this, url, message, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        if (which == DialogInterface.BUTTON_POSITIVE) 
                        {
                            receiver.confirm();
                        }
                        else 
                        {
                            receiver.cancel();
                        }

                    }
                }, 2, "Yes", "No", null).show();                
            }

            @Override
            public void onShowCustomView(View view, WebChromeClient.CustomViewCallback callback) {
                getWindow().setFlags(
                        WindowManager.LayoutParams.FLAG_FULLSCREEN,
                        WindowManager.LayoutParams.FLAG_FULLSCREEN);

                getWindow().addContentView(view,
                        new FrameLayout.LayoutParams(
                                ViewGroup.LayoutParams.MATCH_PARENT,
                                ViewGroup.LayoutParams.MATCH_PARENT,
                                Gravity.CENTER));
                mCustomView = view;
            }

            @Override
            public void onHideCustomView() {
                getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
                FrameLayout decorView = (FrameLayout) getWindow().getDecorView();
                decorView.removeView(mCustomView);
                mCustomView = null;
            }

            @Override
            public boolean shouldOverrideKeyEvent(KeyEvent event) {
                if (event.getKeyCode() == KeyEvent.KEYCODE_BACK) {
                    return true;
                }
                return false;
            }

            @Override
            public void onGeolocationPermissionsShowPrompt(String origin,
                    GeolocationPermissions.Callback callback) {
                callback.invoke(origin, false, false);
            }

            @Override
            public void onReceivedError(int errorCode, String description, String failingUrl) { 
                Utils.createAlertDialog(AwShellActivity.this, "ERROR: " + errorCode, failingUrl + ": " + description, null, 1, "Ok", null, null).show();                
            }

            // @Override
            // public void onReceivedError2(AwWebResourceRequest request, AwWebResourceError error) {
            //     String failingUrl = request.url;
            //     int errorCode = error.errorCode;
            //     String description = error.description;
            //     Utils.createAlertDialog(AwShellActivity.this, "ERROR: " + errorCode, failingUrl + ": " + description, null, 1, "Ok", null, null).show();                
            // }

            @Override
            public void onReceivedHttpError(AwWebResourceRequest request, AwWebResourceResponse response) {
                String failingUrl = request.url;
                // HACK! Do not show the icon loading error.
                if (failingUrl.toLowerCase().contains("favicon.ico")) return;
                int errorCode = response.getStatusCode();
                String description = response.getReasonPhrase();
                Utils.createAlertDialog(AwShellActivity.this, "HTTP ERROR: " + errorCode, failingUrl + ": " + description, null, 1, "Ok", null, null).show();                
            }
        };

        SharedPreferences sharedPreferences =
                getSharedPreferences(PREFERENCES_NAME, Context.MODE_PRIVATE);
        if (mBrowserContext == null) {
            mBrowserContext = new AwBrowserContext(sharedPreferences, getApplicationContext());
        }
        final AwSettings awSettings = new AwSettings(this /* context */,
                false /* isAccessFromFileURLsGrantedByDefault */, false /* supportsLegacyQuirks */,
                false /* allowEmptyDocumentPersistence */,
                true /* allowGeolocationOnInsecureOrigins */);
        // Required for WebGL conformance tests.
        awSettings.setMediaPlaybackRequiresUserGesture(false);
        // Allow zoom and fit contents to screen
        awSettings.setBuiltInZoomControls(false);
        awSettings.setDisplayZoomControls(false);
        awSettings.setUseWideViewPort(true);
        awSettings.setLoadWithOverviewMode(true);
        awSettings.setAllowFileAccessFromFileURLs(true);
        awSettings.setAllowUniversalAccessFromFileURLs(true);
        awSettings.setLayoutAlgorithm(android.webkit.WebSettings.LayoutAlgorithm.TEXT_AUTOSIZING);

        testContainerView.initialize(new AwContents(mBrowserContext, testContainerView,
                testContainerView.getContext(), testContainerView.getInternalAccessDelegate(),
                testContainerView.getNativeDrawGLFunctorFactory(), awContentsClient, awSettings));
        testContainerView.getAwContents().getSettings().setJavaScriptEnabled(true);
        if (mDevToolsServer == null) {
            mDevToolsServer = new AwDevToolsServer();
            mDevToolsServer.setRemoteDebuggingEnabled(true);
        }
        return testContainerView;
    }

    private static String getUrlFromIntent(Intent intent) {
        return intent != null ? intent.getDataString() : null;
    }

    private static String getConfigStringFromIntent(Intent intent) {
        String configString = null;
        if (intent != null) 
        {
            Bundle extras = intent.getExtras();
            if (extras != null) 
            {
                configString = extras.getString("config");
            }
        }
        return configString;
    }

    private void waitForDebuggerIfNeeded() {
        if (CommandLine.getInstance().hasSwitch(BaseSwitches.WAIT_FOR_JAVA_DEBUGGER)) {
            Log.e(TAG, "Waiting for Java debugger to connect...");
            android.os.Debug.waitForDebugger();
            Log.e(TAG, "Java debugger connected. Resuming execution.");
        }
    }

    @Override 
    public boolean dispatchKeyEvent( KeyEvent event )
    {
        if ( nativePointer != 0 )
        {
            int keyCode = event.getKeyCode();
            int action = event.getAction();
            if ( action != KeyEvent.ACTION_DOWN && action != KeyEvent.ACTION_UP )
            {
                return super.dispatchKeyEvent( event );
            }
            if ( action == KeyEvent.ACTION_UP )
            {
//              Log.v( TAG, "GLES3JNIActivity::dispatchKeyEvent( " + keyCode + ", " + action + " )" );
            }
            // nativeOnKeyEvent( nativePointer, keyCode, action );
        }

        return mAwTestContainerView.dispatchKeyEvent(event);
    }

    @Override 
    public boolean dispatchTouchEvent( MotionEvent event )
    {
        if ( nativePointer != 0 )
        {
            int action = event.getAction();
            float x = event.getRawX();
            float y = event.getRawY();
            if ( action == MotionEvent.ACTION_UP )
            {
//              Log.v( TAG, "GLES3JNIActivity::dispatchTouchEvent( " + action + ", " + x + ", " + y + " )" );
            }
            // nativeOnTouchEvent( nativePointer, action, x, y );
        }
        return mAwTestContainerView.dispatchTouchEvent(event);
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent event)
    {
        return mAwTestContainerView.dispatchGenericMotionEvent(event);
    }
    
    @Override public void surfaceCreated( SurfaceHolder holder )
    {
        if ( nativePointer != 0 )
        {
            nativeOnSurfaceCreated( nativePointer, holder.getSurface() );
            surfaceHolder = holder;
        }

        if (!urlLoaded)
        {
            mAwTestContainerView.getAwContents().loadUrl(url);
            urlLoaded = true;
        }

        surfaceView.setZOrderMediaOverlay(true);

        System.out.println("VRWebGL: surfaceCreated and page loaded: " + layout.getMeasuredWidth() + "x" + layout.getMeasuredHeight());
    }

    @Override public void surfaceChanged( SurfaceHolder holder, int format, int width, int height )
    {
        if ( nativePointer != 0 )
        {
            nativeOnSurfaceChanged( nativePointer, holder.getSurface() );
            surfaceHolder = holder;
        }
    }
    
    @Override public void surfaceDestroyed( SurfaceHolder holder )
    {
        if ( nativePointer != 0 )
        {
            nativeOnSurfaceDestroyed( nativePointer );
            surfaceHolder = null;
        }
    }

    @Override
    public void onAudioFocusChange(int focusChange)
    {
        switch( focusChange ) {
        case AudioManager.AUDIOFOCUS_GAIN:
            // resume() if coming back from transient loss, raise stream volume if duck applied
            Log.d(TAG, "onAudioFocusChangedListener: AUDIOFOCUS_GAIN");
            break;
        case AudioManager.AUDIOFOCUS_LOSS:              // focus lost permanently
            // stop() if isPlaying
            Log.d(TAG, "onAudioFocusChangedListener: AUDIOFOCUS_LOSS");     
            break;
        case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:    // focus lost temporarily
            // pause() if isPlaying
            Log.d(TAG, "onAudioFocusChangedListener: AUDIOFOCUS_LOSS_TRANSIENT");   
            break;
        case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:   // focus lost temporarily
            // lower stream volume
            Log.d(TAG, "onAudioFocusChangedListener: AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK");      
            break;
        default:
            break;
        }
    }

    @Override
    public boolean onError(MediaPlayer mp, int what, int extra)
    {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public void onCompletion(MediaPlayer mediaPlayer)
    {
        Log.v(TAG, String.format("onCompletion"));
        Video video = findVideoByMediaPlayer(mediaPlayer);
        nativeVideoEnded(nativePointer, video.nativeTextureId);
    }

    @Override
    public void onVideoSizeChanged(MediaPlayer mp, int width, int height)
    {
    }

    @Override
    public void onFrameAvailable(SurfaceTexture surfaceTexture)
    {
    }

    @Override
    public void onPrepared(MediaPlayer mediaPlayer)
    {
        Log.v(TAG, String.format("onPrepared"));
        Video video = findVideoByMediaPlayer(mediaPlayer);
        nativeVideoPrepared(nativePointer, video.nativeTextureId);
    }
    
    // The video related API. Most of these methods will be called from the native side
    private void requestAudioFocus()
    {
        // Request audio focus
        int result = audioManager.requestAudioFocus( this, AudioManager.STREAM_MUSIC,
            AudioManager.AUDIOFOCUS_GAIN );
        if ( result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED ) 
        {
            Log.d(TAG,"startVideo(): GRANTED audio focus");
        }
        else if ( result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED ) 
        {
            Log.d(TAG,"startVideo(): FAILED to gain audio focus");
        }
    }
    
    private void releaseAudioFocus()
    {
        audioManager.abandonAudioFocus( this );
    }
    
    public void stopVideo(SurfaceTexture surfaceTexture) {
        Log.d(TAG, "stopVideo()" );
        Video video = findVideoBySurfaceTexture(surfaceTexture);
        if (video.mediaPlayer != null) {
            Log.d(TAG, "video stopped" );
            video.mediaPlayer.stop();
        }
        releaseAudioFocus();
    }

    public void setVideoVolume(SurfaceTexture surfaceTexture, float volume)
    {
        Video video = findVideoBySurfaceTexture(surfaceTexture);
        Log.v(TAG, "mediaPlayer.setVolume");
        video.mediaPlayer.setVolume(volume, volume);
    }

    public void setVideoLoop(SurfaceTexture surfaceTexture, boolean loop)
    {
        Video video = findVideoBySurfaceTexture(surfaceTexture);
        Log.v(TAG, "mediaPlayer.setLooping");
        video.mediaPlayer.setLooping(loop);
    }

    public int getVideoWidth(SurfaceTexture surfaceTexture)
    {
        Video video = findVideoBySurfaceTexture(surfaceTexture);
        Log.v(TAG, "mediaPlayer.getVideoWidth");
        return video.mediaPlayer.getVideoWidth();
    }

    public int getVideoHeight(SurfaceTexture surfaceTexture)
    {
        Video video = findVideoBySurfaceTexture(surfaceTexture);
        Log.v(TAG, "mediaPlayer.getVideoHeight");
        return video.mediaPlayer.getVideoHeight();
    }

    public int getVideoDuration(SurfaceTexture surfaceTexture)
    {
        Video video = findVideoBySurfaceTexture(surfaceTexture);
        Log.v(TAG, "mediaPlayer.getDuration");
        return video.mediaPlayer.getDuration();
    }

    public int getVideoCurrentTime(SurfaceTexture surfaceTexture)
    {
        Video video = findVideoBySurfaceTexture(surfaceTexture);
        Log.v(TAG, "mediaPlayer.getPosition");
        return video.mediaPlayer.getCurrentPosition();
    }

    public void pauseVideo(SurfaceTexture surfaceTexture) {
        Log.d(TAG, "pauseVideo()" );
        try {
            Video video = findVideoBySurfaceTexture(surfaceTexture);
            if (video.mediaPlayer != null) {
                Log.d(TAG, "video paused" );
                video.mediaPlayer.pause();
            }
        }
        catch( IllegalStateException ise ) {
            Log.d( TAG, "pauseVideo(): Caught illegalStateException: " + ise.toString() );
        }
    }

    public void seekTo( SurfaceTexture surfaceTexture, final int seekPos ) {
        Log.d( TAG, "seekToFromNative to " + seekPos );
        try {
            Video video = findVideoBySurfaceTexture(surfaceTexture);
            if (video.mediaPlayer != null) {
                video.mediaPlayer.seekTo(seekPos);
                try {
                    Log.v(TAG, "mediaPlayer.prepare");
                    video.mediaPlayer.prepare();
                } catch (IOException t) {
                    Log.e(TAG, "mediaPlayer.prepare failed:" + t.getMessage());
                }
            }
        }
        catch( IllegalStateException ise ) {
            Log.d( TAG, "seekToFromNative(): Caught illegalStateException: " + ise.toString() );
        }
    }

    public void playVideoOnUIThread( final SurfaceTexture surfaceTexture, final float volume, final boolean loop ) {
        Log.d( TAG, "playVideoOnUIThread" );
        runOnUiThread( new Runnable() {
            @Override
            public void run() {
                playVideo( surfaceTexture, volume, loop );
            }
        });
    }

    public void playVideo(SurfaceTexture surfaceTexture, float volume, boolean loop) {
        Log.v(TAG, "playVideo ");
        synchronized(this)
        {        
            Video video = findVideoBySurfaceTexture(surfaceTexture);

            Log.v(TAG, "mediaPlayer.start");
            try {
                video.mediaPlayer.start();
            }
            catch( IllegalStateException ise ) {
                Log.d( TAG, "mediaPlayer.start(): Caught illegalStateException: " + ise.toString() );
            }
            
            video.mediaPlayer.setVolume(volume, volume);
            video.mediaPlayer.setLooping(loop);
        }

        Log.v(TAG, "returning");
    }       

    public void setVideoSrcOnUIThread( final SurfaceTexture surfaceTexture, final String src) {
        Log.d( TAG, "setVideoSrcOnUIThread" );
        runOnUiThread( new Runnable() {
            @Override
            public void run() {
                setVideoSrc( surfaceTexture, src );
            }
        });
    }

    public void setVideoSrc(SurfaceTexture surfaceTexture, String src) {
        Log.v(TAG, "setVideoSrc " + src);
        synchronized (this) 
        {
            // Request audio focus
            requestAudioFocus();
        
            Video video = findVideoBySurfaceTexture(surfaceTexture);

            System.out.println("VRWebGL: setVideoSrc " + surfaceTexture + ", " + video);
        
            try {
                Log.v(TAG, "mediaPlayer.setDataSource()");
                
                if (src.contains("file:///android_asset/"))
                {
                    src = src.replace("file:///android_asset/", "");
                    src = src.replace("index.html", "");
                    System.out.println("VRWebGL: src = " + src);
                    AssetFileDescriptor assetFileDescriptor = getAssets().openFd(src);
                    video.mediaPlayer.setDataSource(assetFileDescriptor.getFileDescriptor(), assetFileDescriptor.getStartOffset(), assetFileDescriptor.getLength());
                }
                else 
                {
                    video.mediaPlayer.setDataSource(src);
                }
            } catch (IOException t) {
                Log.e(TAG, "mediaPlayer.setDataSource failed");
            }

            try {
                Log.v(TAG, "mediaPlayer.prepare");
                video.mediaPlayer.prepare();
            } catch (IOException t) {
                Log.e(TAG, "mediaPlayer.prepare failed:" + t.getMessage());
            }
        }        
    }


    public synchronized void newVideo(SurfaceTexture surfaceTexture, int nativeTextureId) 
    {
        System.out.println("VRWebGL: newVideo " + surfaceTexture);

        Video video = new Video();
        // Have native code pause any playing video,
        // allocate a new external texture,
        // and create a surfaceTexture with it.
        video.surfaceTexture = surfaceTexture;
        video.surfaceTexture.setOnFrameAvailableListener(this);
        video.surface = new Surface(video.surfaceTexture);

        video.nativeTextureId = nativeTextureId;

        Log.v(TAG, "MediaPlayer.create");

        synchronized (this) {
            video.mediaPlayer = new MediaPlayer();
        }
        video.mediaPlayer.setOnVideoSizeChangedListener(this);
        video.mediaPlayer.setOnCompletionListener(this);
        video.mediaPlayer.setOnPreparedListener(this);
        video.mediaPlayer.setSurface(video.surface);

        synchronized(this)
        {
            videos.add(video);
        }
    }

    public synchronized void deleteVideo(SurfaceTexture surfaceTexture) 
    {
        Video video = findVideoBySurfaceTexture(surfaceTexture);
        video.mediaPlayer.release();
        videos.remove(video);        
    }

    private synchronized Video findVideoBySurfaceTexture(SurfaceTexture surfaceTexture)
    {
        Video video = null;
        for (int i = 0; i < videos.size() && video == null; i++)
        {
            video = videos.get(i);
            if (video.surfaceTexture != surfaceTexture)
            {
                video = null;
            }
        }
        return video;
    }

    private synchronized Video findVideoByMediaPlayer(MediaPlayer mediaPlayer)
    {
        Video video = null;
        for (int i = 0; i < videos.size() && video == null; i++)
        {
            video = videos.get(i);
            if (video.mediaPlayer != mediaPlayer)
            {
                video = null;
            }
        }
        return video;
    }


    // Native calls
    
    // Activity lifecycle
    private native long nativeOnCreate( Activity obj );
    private native void nativeOnStart( long nativePointer );
    private native void nativeOnResume( long nativePointer );
    private native void nativeOnPause( long nativePointer );
    private native void nativeOnStop( long nativePointer );
    private native void nativeOnDestroy( long nativePointer );

    private native void nativeOnPageStarted();

    // Surface lifecycle
    public native void nativeOnSurfaceCreated( long nativePointer, Surface s );
    public native void nativeOnSurfaceChanged( long nativePointer, Surface s );
    public native void nativeOnSurfaceDestroyed( long nativePointer );

    // Input
    private native void nativeOnKeyEvent( long nativePointer, int keyCode, int action );
    private native void nativeOnTouchEvent( long nativePointer, int action, float x, float y );    

    // Video
    private native void nativeVideoPrepared(long nativePointer, int textureId);
    private native void nativeVideoEnded(long nativePointer, int textureId);

    private void logHeapUsageFromNative()
    {
        double allocated = (double)Debug.getNativeHeapAllocatedSize()/1048576.0;
        double available = (double)Debug.getNativeHeapSize()/1048576.0;
        double free = (double)Debug.getNativeHeapFreeSize()/1048576.0;
        DecimalFormat df = new DecimalFormat();
        df.setMaximumFractionDigits(2);
        df.setMinimumFractionDigits(2);

        Log.d("VRWebGL", "VRWebGL: debug. =================================");
        Log.d("VRWebGL", "VRWebGL: debug.heap native: allocated " + df.format(allocated) + "MB of " + df.format(available) + "MB (" + df.format(free) + "MB free)");
        Log.d("VRWebGL", "VRWebGL: debug.memory: allocated: " + df.format((double)Runtime.getRuntime().totalMemory()/1048576.0) + "MB of " + df.format((double)Runtime.getRuntime().maxMemory()/1048576.0)+ "MB (" + df.format((double)Runtime.getRuntime().freeMemory()/1048576.0) +"MB free)");
    }
}
