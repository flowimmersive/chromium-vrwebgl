if (self.importScripts) {
  importScripts('/fetch/resources/fetch-test-helpers.js');
  importScripts('/fetch/resources/thorough-util.js');
}

var TEST_TARGETS = [
  // Redirect: same origin -> same origin
  [REDIRECT_URL + encodeURIComponent(BASE_URL) +
   '&mode=same-origin&method=GET',
   [fetchResolved, hasContentLength, hasServerHeader, hasBody, typeBasic,
    responseRedirected, checkURLList.bind(self, [BASE_URL])],
   [methodIsGET, authCheck1]],

  // https://fetch.spec.whatwg.org/#concept-http-fetch
  // Step 4, Case 301/302/303/307/308:
  // Step 2: If location is null, return response.
  [REDIRECT_URL + 'noLocation' +
   '&mode=same-origin&method=GET&NoRedirectTest=true',
   [fetchResolved, hasBody, typeBasic, responseNotRedirected,
    checkURLList.bind(self, [])],
   [checkJsonpNoRedirect]],
  // Step 5: If locationURL is failure, return a network error.
  [REDIRECT_URL + 'http://' +
   '&mode=same-origin&method=GET',
   [fetchRejected]],

  [REDIRECT_URL + encodeURIComponent(BASE_URL) +
   '&mode=same-origin&method=GET&headers=CUSTOM',
   [fetchResolved, hasContentLength, hasServerHeader, hasBody, typeBasic,
    responseRedirected, checkURLList.bind(self, [BASE_URL])],
   [methodIsGET, hasCustomHeader, authCheck1]],
  // Chrome changes the method from POST to GET when it recieves 301 redirect
  // response. See a note in http://tools.ietf.org/html/rfc7231#section-6.4.2
  [REDIRECT_URL + encodeURIComponent(BASE_URL) +
   '&mode=same-origin&method=POST&Status=301',
   [fetchResolved, hasContentLength, hasServerHeader, hasBody, typeBasic,
    responseRedirected, checkURLList.bind(self, [BASE_URL])],
   [methodIsGET, authCheck1]],
  // Chrome changes the method from POST to GET when it recieves 302 redirect
  // response. See a note in http://tools.ietf.org/html/rfc7231#section-6.4.3
  [REDIRECT_URL + encodeURIComponent(BASE_URL) +
   '&mode=same-origin&method=POST',
   [fetchResolved, hasContentLength, hasServerHeader, hasBody, typeBasic,
    responseRedirected, checkURLList.bind(self, [BASE_URL])],
   [methodIsGET, authCheck1]],
  // GET method must be used for 303 redirect.
  [REDIRECT_URL + encodeURIComponent(BASE_URL) +
   '&mode=same-origin&method=POST&Status=303',
   [fetchResolved, hasContentLength, hasServerHeader, hasBody, typeBasic,
    responseRedirected, checkURLList.bind(self, [BASE_URL])],
   [methodIsGET, authCheck1]],
  // The 307 redirect response doesn't change the method.
  [REDIRECT_URL + encodeURIComponent(BASE_URL) +
   '&mode=same-origin&method=POST&Status=307',
   [fetchResolved, hasContentLength, hasServerHeader, hasBody, typeBasic,
    responseRedirected, checkURLList.bind(self, [BASE_URL])],
   [methodIsPOST, authCheck1]],
  // The 308 redirect response doesn't change the method.
  // FIXME: currently this and following 308 tests are disabled because they
  // fail on try bots, probably due to Apache/PHP versions.
  // https://crbug.com/451938
  // [REDIRECT_URL + encodeURIComponent(BASE_URL) +
  //  '&mode=same-origin&method=POST&Status=308',
  //  [fetchResolved, hasContentLength, hasServerHeader, hasBody, typeBasic],
  //  [methodIsPOST, authCheck1]],

  // Do not redirect for other status even if Location header exists.
  [REDIRECT_URL + encodeURIComponent(BASE_URL) +
   '&mode=same-origin&credentials=same-origin&method=POST&Status=201&' +
   'NoRedirectTest=true',
   [fetchResolved, hasBody, typeBasic,
    responseNotRedirected, checkURLList.bind(self, [])],
   [checkJsonpNoRedirect]],

  [REDIRECT_URL + encodeURIComponent(BASE_URL) +
   '&mode=same-origin&credentials=same-origin&method=PUT',
   [fetchResolved, hasContentLength, hasServerHeader, hasBody, typeBasic,
    responseRedirected, checkURLList.bind(self, [BASE_URL])],
   [methodIsPUT, authCheck1]],

  [REDIRECT_URL + encodeURIComponent(BASE_URL) +
   '&mode=cors&credentials=same-origin&method=GET&headers=CUSTOM',
   [fetchResolved, hasContentLength, hasServerHeader, hasBody, typeBasic,
    responseRedirected, checkURLList.bind(self, [BASE_URL])],
   [methodIsGET, hasCustomHeader, authCheck1]],

  // Redirect: same origin -> other origin
  [REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL) +
   '&mode=same-origin&credentials=same-origin&method=GET',
   [fetchRejected]],
  [REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL) +
   '&mode=same-origin&credentials=same-origin&method=POST',
   [fetchRejected]],
  [REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL) +
   '&mode=same-origin&credentials=same-origin&method=PUT',
   [fetchRejected]],

  [REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL) +
   '&mode=cors&credentials=same-origin&method=GET',
   [fetchRejected]],
  [REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL) +
   '&mode=cors&credentials=same-origin&method=PUT',
   [fetchRejected]],

  [REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL + '&ACAOrigin=*') +
   '&mode=cors&credentials=same-origin&method=GET',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors,
    responseRedirected,
    checkURLList.bind(self, [OTHER_BASE_URL + '&ACAOrigin=*'])],
   [methodIsGET, authCheckNone]],
  [REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL + '&ACAOrigin=*') +
   '&mode=cors&credentials=same-origin&method=PUT',
   [fetchRejected]],
  [REDIRECT_URL +
   encodeURIComponent(OTHER_BASE_URL + '&ACAOrigin=*&ACAMethods=PUT') +
   '&mode=cors&credentials=same-origin&method=PUT',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors,
    responseRedirected,
    checkURLList.bind(self, [OTHER_BASE_URL + '&ACAOrigin=*&ACAMethods=PUT'])],
   [methodIsPUT, noCustomHeader, authCheckNone]],

  // Status code tests for mode="cors"
  // The 301 redirect response MAY change the request method from POST to GET.
  [REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL + '&ACAOrigin=*') +
   '&mode=cors&credentials=same-origin&method=POST&Status=301',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors,
    responseRedirected,
    checkURLList.bind(self, [OTHER_BASE_URL + '&ACAOrigin=*'])],
   [methodIsGET]],
  // The 302 redirect response MAY change the request method from POST to GET.
  [REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL + '&ACAOrigin=*') +
   '&mode=cors&credentials=same-origin&method=POST&Status=302',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors,
    responseRedirected,
    checkURLList.bind(self, [OTHER_BASE_URL + '&ACAOrigin=*'])],
   [methodIsGET]],
  // GET method must be used for 303 redirect.
  [REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL + '&ACAOrigin=*') +
   '&mode=cors&credentials=same-origin&method=POST&Status=303',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors,
    responseRedirected,
    checkURLList.bind(self, [OTHER_BASE_URL + '&ACAOrigin=*'])],
   [methodIsGET]],
  // The 307 redirect response MUST NOT change the method.
  [REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL + '&ACAOrigin=*') +
   '&mode=cors&credentials=same-origin&method=POST&Status=307',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors,
    responseRedirected,
    checkURLList.bind(self, [OTHER_BASE_URL + '&ACAOrigin=*'])],
   [methodIsPOST]],
  // The 308 redirect response MUST NOT change the method.
  // FIXME: disabled due to https://crbug.com/451938
  // [REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL + '&ACAOrigin=*') +
  //  '&mode=cors&credentials=same-origin&method=POST&Status=308',
  //  [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors],
  //  [methodIsPOST]],

  // Custom header
  [REDIRECT_URL +
   encodeURIComponent(
     OTHER_BASE_URL +
     'ACAOrigin=*&ACAHeaders=x-serviceworker-test') +
   '&mode=cors&credentials=same-origin&method=GET&headers=CUSTOM',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors,
    responseRedirected,
    checkURLList.bind(
        self,
        [OTHER_BASE_URL + 'ACAOrigin=*&ACAHeaders=x-serviceworker-test'])],
   [methodIsGET, hasCustomHeader, authCheckNone]],
  [REDIRECT_URL +
   encodeURIComponent(OTHER_BASE_URL +'ACAOrigin=*') +
   '&mode=cors&credentials=same-origin&method=GET&headers=CUSTOM',
   [fetchRejected]],
  [REDIRECT_URL +
   encodeURIComponent(OTHER_BASE_URL + 'ACAHeaders=x-serviceworker-test') +
   '&mode=cors&credentials=same-origin&method=GET&headers=CUSTOM',
   [fetchRejected]],

  // Server header
  [REDIRECT_URL +
   encodeURIComponent(
     OTHER_BASE_URL +
     '&ACAOrigin=' + BASE_ORIGIN +
     '&ACEHeaders=Content-Length, X-ServiceWorker-ServerHeader') +
   '&mode=cors&credentials=same-origin&method=GET',
   [fetchResolved, hasContentLength, hasServerHeader, hasBody, typeCors,
    responseRedirected,
    checkURLList.bind(
        self,
        [OTHER_BASE_URL +'&ACAOrigin=' + BASE_ORIGIN +
         '&ACEHeaders=Content-Length,%20X-ServiceWorker-ServerHeader'])],
   [methodIsGET, authCheckNone]],

  // Redirect: other origin -> same origin
  [OTHER_REDIRECT_URL + encodeURIComponent(BASE_URL) +
   '&mode=same-origin&credentials=same-origin&method=GET',
   [fetchRejected]],
  [OTHER_REDIRECT_URL + encodeURIComponent(BASE_URL) +
   '&mode=same-origin&credentials=same-origin&method=POST',
   [fetchRejected]],

  [OTHER_REDIRECT_URL + encodeURIComponent(BASE_URL) +
   '&mode=cors&credentials=same-origin&method=GET',
   [fetchRejected]],
  [OTHER_REDIRECT_URL + encodeURIComponent(BASE_URL) +
   '&mode=cors&credentials=same-origin&method=GET&ACAOrigin=*',
   [fetchRejected]],
  [OTHER_REDIRECT_URL + encodeURIComponent(BASE_URL + 'ACAOrigin=*') +
   '&mode=cors&credentials=same-origin&method=GET&ACAOrigin=*',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors,
    responseRedirected,
    checkURLList.bind(self, [BASE_URL + 'ACAOrigin=*'])],
   [methodIsGET, authCheckNone]],

  // Status code tests for mode="cors"
  // The 301 redirect response MAY change the request method from POST to GET.
  [OTHER_REDIRECT_URL + encodeURIComponent(BASE_URL + 'ACAOrigin=*') +
   '&mode=cors&credentials=same-origin&method=post&ACAOrigin=*&Status=301',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors,
    responseRedirected, checkURLList.bind(self, [BASE_URL + 'ACAOrigin=*'])],
   [methodIsGET]],
  // The 302 redirect response MAY change the request method from POST to GET.
  [OTHER_REDIRECT_URL + encodeURIComponent(BASE_URL + 'ACAOrigin=*') +
   '&mode=cors&credentials=same-origin&method=post&ACAOrigin=*&Status=302',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors,
    responseRedirected, checkURLList.bind(self, [BASE_URL + 'ACAOrigin=*'])],
   [methodIsGET]],
  // GET method must be used for 303 redirect.
  [OTHER_REDIRECT_URL + encodeURIComponent(BASE_URL + 'ACAOrigin=*') +
   '&mode=cors&credentials=same-origin&method=post&ACAOrigin=*&Status=303',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors,
    responseRedirected, checkURLList.bind(self, [BASE_URL + 'ACAOrigin=*'])],
   [methodIsGET]],
  // The 307 redirect response MUST NOT change the method.
  [OTHER_REDIRECT_URL + encodeURIComponent(BASE_URL + 'ACAOrigin=*') +
   '&mode=cors&credentials=same-origin&method=post&ACAOrigin=*&Status=307',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors,
    responseRedirected, checkURLList.bind(self, [BASE_URL + 'ACAOrigin=*'])],
   [methodIsPOST]],
  // The 308 redirect response MUST NOT change the method.
  // FIXME: disabled due to https://crbug.com/451938
  // [OTHER_REDIRECT_URL + encodeURIComponent(BASE_URL + 'ACAOrigin=*') +
  //  '&mode=cors&credentials=same-origin&method=post&ACAOrigin=*&Status=308',
  //  [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors],
  //  [methodIsPOST]],

  // Custom method
  [OTHER_REDIRECT_URL +
   encodeURIComponent(BASE_URL + 'ACAOrigin=*&ACAMethods=PUT') +
   '&mode=cors&credentials=same-origin&method=PUT&ACAOrigin=*&ACAMethods=PUT',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors],
   [methodIsPUT, authCheckNone]],
  // Custom header
  [OTHER_REDIRECT_URL +
   encodeURIComponent(
       BASE_URL +
       'ACAOrigin=*&ACAHeaders=x-serviceworker-test') +
   '&mode=cors&credentials=same-origin&method=GET&headers=CUSTOM' +
   '&ACAOrigin=*&ACAHeaders=x-serviceworker-test',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors],
   [methodIsGET, hasCustomHeader, authCheckNone]],

  // Redirect: other origin -> other origin
  [OTHER_REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL) +
   '&mode=same-origin&method=GET',
   [fetchRejected]],
  [OTHER_REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL) +
   '&mode=cors&credentials=same-origin&method=GET',
   [fetchRejected]],
  [OTHER_REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL) +
   '&mode=cors&credentials=same-origin&method=GET&ACAOrigin=*',
   [fetchRejected]],
  [OTHER_REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL + 'ACAOrigin=*') +
   '&mode=cors&credentials=same-origin&method=GET&ACAOrigin=*',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors,
    responseRedirected,
    checkURLList.bind(self, [OTHER_BASE_URL + 'ACAOrigin=*'])],
   [methodIsGET, authCheckNone]],
  [OTHER_REDIRECT_URL +
   encodeURIComponent(OTHER_BASE_URL + 'ACAOrigin=' + BASE_ORIGIN + '') +
   '&mode=cors&credentials=same-origin&method=GET&ACAOrigin=*',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors,
    responseRedirected,
    checkURLList.bind(self, [OTHER_BASE_URL + 'ACAOrigin=' + BASE_ORIGIN])],
   [methodIsGET, authCheckNone]],
  [OTHER_REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL + 'ACAOrigin=*') +
   '&mode=cors&credentials=same-origin&method=GET' +
   '&ACAOrigin=' + BASE_ORIGIN + '',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors,
    responseRedirected,
    checkURLList.bind(self, [OTHER_BASE_URL + 'ACAOrigin=*'])],
   [methodIsGET, authCheckNone]],
  [OTHER_REDIRECT_URL +
   encodeURIComponent(OTHER_BASE_URL + 'ACAOrigin=' + BASE_ORIGIN + '') +
   '&mode=cors&credentials=same-origin&method=GET' +
   '&ACAOrigin=' + BASE_ORIGIN + '',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors,
    responseRedirected,
    checkURLList.bind(self, [OTHER_BASE_URL + 'ACAOrigin=' + BASE_ORIGIN])],
   [methodIsGET, authCheckNone]],

  // Status code tests for mode="cors"
  // The 301 redirect response MAY change the request method from POST to GET.
  [OTHER_REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL + 'ACAOrigin=*') +
   '&mode=cors&credentials=same-origin&method=POST&ACAOrigin=*&Status=301',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors,
    responseRedirected,
    checkURLList.bind(self, [OTHER_BASE_URL + 'ACAOrigin=*'])],
   [methodIsGET]],
  // The 302 redirect response MAY change the request method from POST to GET.
  [OTHER_REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL + 'ACAOrigin=*') +
   '&mode=cors&credentials=same-origin&method=POST&ACAOrigin=*&Status=302',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors,
    responseRedirected,
    checkURLList.bind(self, [OTHER_BASE_URL + 'ACAOrigin=*'])],
   [methodIsGET]],
  // GET method must be used for 303 redirect.
  [OTHER_REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL + 'ACAOrigin=*') +
   '&mode=cors&credentials=same-origin&method=POST&ACAOrigin=*&Status=303',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors,
    responseRedirected,
    checkURLList.bind(self, [OTHER_BASE_URL + 'ACAOrigin=*'])],
   [methodIsGET]],
  // The 307 redirect response MUST NOT change the method.
  [OTHER_REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL + 'ACAOrigin=*') +
   '&mode=cors&credentials=same-origin&method=POST&ACAOrigin=*&Status=307',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors,
    responseRedirected,
    checkURLList.bind(self, [OTHER_BASE_URL + 'ACAOrigin=*'])],
   [methodIsPOST]],
  // The 308 redirect response MUST NOT change the method.
  // FIXME: disabled due to https://crbug.com/451938
  // [OTHER_REDIRECT_URL + encodeURIComponent(OTHER_BASE_URL + 'ACAOrigin=*') +
  //  '&mode=cors&credentials=same-origin&method=POST&ACAOrigin=*&Status=308',
  //  [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors],
  //  [methodIsPOST]],

  // Server header
  [OTHER_REDIRECT_URL +
   encodeURIComponent(OTHER_BASE_URL +
                      'ACAOrigin=*&ACEHeaders=X-ServiceWorker-ServerHeader') +
   '&mode=cors&credentials=same-origin&method=GET&ACAOrigin=*',
   [fetchResolved, noContentLength, hasServerHeader, hasBody, typeCors,
    responseRedirected,
    checkURLList.bind(
        self,
        [OTHER_BASE_URL +
         'ACAOrigin=*&ACEHeaders=X-ServiceWorker-ServerHeader'])],
   [methodIsGET, authCheckNone]],

  // Custom method
  [OTHER_REDIRECT_URL +
   encodeURIComponent(OTHER_BASE_URL + 'ACAOrigin=*&ACAMethods=PUT') +
   '&mode=cors&credentials=same-origin&method=PUT&ACAOrigin=*&ACAMethods=PUT',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors],
   [methodIsPUT, authCheckNone]],
  // Custom header
  [OTHER_REDIRECT_URL +
   encodeURIComponent(
     OTHER_BASE_URL +
     'ACAOrigin=' + BASE_ORIGIN + '&ACAHeaders=x-serviceworker-test') +
   '&mode=cors&credentials=same-origin&method=GET&headers=CUSTOM' +
   '&ACAOrigin=' + BASE_ORIGIN + '&ACAHeaders=x-serviceworker-test',
   [fetchResolved, noContentLength, noServerHeader, hasBody, typeCors],
   [methodIsGET, hasCustomHeader, authCheckNone]],
];

if (self.importScripts) {
  executeTests(TEST_TARGETS);
  done();
}
