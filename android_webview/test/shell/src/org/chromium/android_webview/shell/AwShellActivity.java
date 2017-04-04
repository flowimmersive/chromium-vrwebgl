// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.shell;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.res.AssetFileDescriptor;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Debug;
import android.text.TextUtils;
import android.text.InputType;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.View.OnClickListener;
import android.view.View.OnFocusChangeListener;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.webkit.GeolocationPermissions;
import android.webkit.WebChromeClient;
import android.webkit.WebViewClient;
import android.webkit.JavascriptInterface;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;

import android.support.v4.content.ContextCompat;
import android.support.v4.app.ActivityCompat;
import android.support.v4.app.ActivityCompat.OnRequestPermissionsResultCallback;

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

import android.opengl.GLSurfaceView;
import android.view.Surface;

import java.net.URI;
import java.net.URISyntaxException;
import java.net.URLEncoder;
import android.app.AlertDialog;
import android.content.DialogInterface;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;
import android.widget.FrameLayout;
import java.text.DecimalFormat;

import java.util.ArrayList;

import org.json.JSONException;
import org.json.JSONObject;

import android.media.AudioManager;
import android.media.MediaPlayer;
import android.graphics.SurfaceTexture;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.Bitmap;

import com.google.vr.ndk.base.AndroidCompat;
import com.google.vr.ndk.base.GvrLayout;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.speech.SpeechRecognizer;
import android.speech.RecognizerIntent;
import android.speech.RecognitionListener;

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
        GLSurfaceView.Renderer {  
    private static final String TAG = "cr.AwShellActivity";
    private static final String PREFERENCES_NAME = "AwShellPrefs";
    private static final String INITIAL_URL = "No URL provided either in the intent nor in the config.json";
    private static final String LAST_USED_URL_KEY = "url";
    private static final int READ_EXTERNAL_STORAGE_PERMISSION_ID = 3;
    private static final int RECORD_AUDIO_PERMISSION_ID = 4;
    private AwBrowserContext mBrowserContext;
    private AwDevToolsServer mDevToolsServer;
    private AwTestContainerView mAwTestContainerView;

    private SpeechRecognizer speechRecognizer;
    private Intent speechRecognizerIntent;
    private boolean recognizingSpeech = false;
    private SpeechRecognitionListener speechRecognizerListener;

    private GLSurfaceView surfaceView;
    private GvrLayout gvrLayout;
    private long nativePointer = 0;   
    // This is done on the GL thread because refreshViewerProfile isn't thread-safe.
    private final Runnable refreshViewerProfileRunnable = new Runnable() {
        @Override
        public void run() {
            gvrLayout.getGvrApi().refreshViewerProfile();
        }
    };
    
    private class Video
    {
        public SurfaceTexture surfaceTexture = null;
        public Surface surface = null;
        public MediaPlayer mediaPlayer = null;
        public int nativeTextureId = -1;
    }
    private ArrayList<Video> videos = new ArrayList<Video>();
    private AudioManager audioManager = null;

    class WebView extends android.webkit.WebView 
    {
        public static final int WEBVIEW_TEXTURE_WIDTH = 960;
        public static final int WEBVIEW_TEXTURE_HEIGHT = 640;

        public static final int TOUCH_START = 1;
        public static final int TOUCH_MOVE = 2;
        public static final int TOUCH_END = 3;

        public static final int NAVIGATION_BACK = 1;
        public static final int NAVIGATION_FORWARD = 2;
        public static final int NAVIGATION_RELOAD = 3;
        public static final int NAVIGATION_VOICE_SEARCH = 4;

        public static final int KEYBOARD_KEY_DOWN = 1;
        public static final int KEYBOARD_KEY_UP = 2;

        public static final int CURSOR_ENTER = 1;
        public static final int CURSOR_MOVE = 2;
        public static final int CURSOR_EXIT = 3;
 
        private Surface surface = null;
        private SurfaceTexture surfaceTexture = null; 
        private int nativeTextureId = -1;
        public MotionEvent.PointerProperties[] pointerProperties = { new MotionEvent.PointerProperties() };
        public MotionEvent.PointerCoords[] pointerCoords = { new MotionEvent.PointerCoords() };
        public VRBrowserJSInterface vrbrowser = null;
        
        public WebView( Context context, SurfaceTexture surfaceTexture, int nativeTextureId ) 
        {
            super( context );

            this.surfaceTexture = surfaceTexture;
            this.surfaceTexture.setDefaultBufferSize(WEBVIEW_TEXTURE_WIDTH, WEBVIEW_TEXTURE_HEIGHT);
            this.surface = new Surface(this.surfaceTexture);
            this.nativeTextureId = nativeTextureId;

            // setBackgroundColor(Color.TRANSPARENT);
            getSettings().setJavaScriptEnabled(true);
            // getSettings().setAllowFileAccess(true);
            // getSettings().setLoadsImagesAutomatically(true);
            // getSettings().setBlockNetworkImage(false);
            // getSettings().setBlockNetworkLoads(false);
            // getSettings().setBuiltInZoomControls(false);
            // getSettings().setLoadWithOverviewMode(true);
            // getSettings().setUseWideViewPort(true);
            // getSettings().setDomStorageEnabled(true);
            clearCache(true);
            setScrollbarFadingEnabled(true);
            // setScrollBarStyle(android.webkit.WebView.SCROLLBARS_OUTSIDE_OVERLAY);
            setVerticalScrollBarEnabled(false);
            setHorizontalScrollBarEnabled(false);
            setNetworkAvailable(true);
            setWebViewClient(new WebViewClient()
            {
                @Override
                public void onPageStarted(android.webkit.WebView view, String url, Bitmap favicon)
                {
                    dispatchEventToVRBrowser("loadstart", "{ url: '" + url + "'}");
                }

                @Override
                public void onPageFinished(android.webkit.WebView view, String url)
                {
                    dispatchEventToVRBrowser("loadend", "{ url: '" + url + "'}");
                }
            });
            setWebChromeClient(new WebChromeClient() 
            {
                @Override
                public void onProgressChanged(android.webkit.WebView view, int progress) 
                {
                    dispatchEventToVRBrowser("loadprogress", "{ url: '" + view.getUrl() + "', progress: " + progress + "}");
                }
            });

            // Disable long click for text selection.
            // TODO: Disable the vibration too.
            setOnLongClickListener(new View.OnLongClickListener() {
                @Override
                public boolean onLongClick(View v) {
                    return true;
                }
            });

            // Inject the vrbrowser instance to allow the webivew content to call/use it.
            vrbrowser = new VRBrowserJSInterface(this);
            addJavascriptInterface(vrbrowser, "vrbrowser");

            setLayoutParams( new ViewGroup.LayoutParams( WEBVIEW_TEXTURE_WIDTH, WEBVIEW_TEXTURE_WIDTH ) );
        }

        @Override
        protected void onDraw( Canvas canvas ) 
        {
            if ( surface != null ) 
            {
                try 
                {                    
                    // final Canvas surfaceCanvas = surface.lockHardwareCanvas();
                    final Canvas surfaceCanvas = surface.lockCanvas( null );

                    surfaceCanvas.save();
                    surfaceCanvas.translate(-getScrollX(), -getScrollY());
                    super.onDraw(surfaceCanvas);
                    surfaceCanvas.restore();

                    surface.unlockCanvasAndPost( surfaceCanvas );
                } 
                catch ( Surface.OutOfResourcesException e ) 
                {
                    e.printStackTrace();
                }    
            }
            // super.onDraw( canvas ); // Uncomment this to render the webview as a 2d view on top of everything.
        }

        public SurfaceTexture getSurfaceTexture()
        {
            return surfaceTexture;
        }

        public int getTextureId()
        {
            return nativeTextureId;
        } 

        private void dispatchEventToVRBrowser(String eventName, String eventDataJSONString)
        {
            String jsCode = "if (window.vrbrowser) window.vrbrowser.dispatchEvent(" + nativeTextureId + ", '" + eventName + "', " + eventDataJSONString + ");";
            mAwTestContainerView.getAwContents().evaluateJavaScript(jsCode, null);
        }

    }
    private ArrayList<WebView> webviews = new ArrayList<WebView>();

    private class SpeechRecognitionListener implements RecognitionListener
    {
        private WebView webview = null;

        public void setWebView(WebView webview)
        {
            this.webview = webview;
        }

        @Override
        public void onBeginningOfSpeech()
        {               
            // System.out.println("SpeechRecognitionListener.onBeginningOfSpeech");
        }

        @Override
        public void onBufferReceived(byte[] buffer)
        {
            // System.out.println("SpeechRecognitionListener.onBufferReceived");
        }

        @Override
        public void onEndOfSpeech()
        {
            // System.out.println("SpeechRecognitionListener.onEndOfSpeech");
         }

        @Override
        public void onError(int error)
        {
             // speechRecognizer.startListening(speechRecognizerIntent);
            // System.out.println("SpeechRecognitionListener.onError: " + error);
        }

        @Override
        public void onEvent(int eventType, Bundle params)
        {
            // System.out.println("SpeechRecognitionListener.onEvent");
        }

        @Override
        public void onPartialResults(Bundle partialResults)
        {
            // System.out.println("SpeechRecognitionListener.onPartialResults");
        }

        @Override
        public void onReadyForSpeech(Bundle params)
        {
            // System.out.println("SpeechRecognitionListener.onReadyForSpeech");
        }

        @Override
        public void onResults(Bundle results)
        {
            // System.out.println("SpeechRecognitionListener.onResults");
            ArrayList<String> matches = results.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION);
            if (webview != null && !matches.isEmpty())
            {
                try
                {
                    String encodedMatch = URLEncoder.encode(matches.get(0), "UTF-8");
                    webview.loadUrl("http://www.google.com/search?q=" + encodedMatch);
                }
                catch(UnsupportedEncodingException e)
                {

                }
            }
        }

        @Override
        public void onRmsChanged(float rmsdB)
        {
            // System.out.println("SpeechRecognitionListener.onRmsChanged");
        }
    }

    private class VRBrowserJSInterface
    {
        private WebView webview = null;

        public VRBrowserJSInterface(WebView webview)
        {
            this.webview = webview;
        }

        @JavascriptInterface
        public void dispatchEvent(String jsonString)
        {
            webview.dispatchEventToVRBrowser("eventfrompage", jsonString);
        }
    }

    private String vrWebGLJSCode; 
    private String url;
    private boolean urlLoaded = false;

    private JSONObject config = null;

    private FrameLayout layout;

    // This is the same as data_reduction_proxy::switches::kEnableDataReductionProxy.
    private static final String ENABLE_DATA_REDUCTION_PROXY = "enable-spdy-proxy-auth";
    // This is the same as data_reduction_proxy::switches::kDataReductionProxyKey.
    private static final String DATA_REDUCTION_PROXY_KEY = "spdy-proxy-auth-value";

    private void setImmersiveSticky() {
        getWindow()
            .getDecorView()
            .setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
    }

    private void requestExternalStorageReadPermission() 
    {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) 
        {
            // Should we show an explanation?
            // if (ActivityCompat.shouldShowRequestPermissionRationale(this, Manifest.permission.READ_EXTERNAL_STORAGE)) 
            // {

            //     // Show an expanation to the user *asynchronously* -- don't block
            //     // this thread waiting for the user's response! After the user
            //     // sees the explanation, try again to request the permission.

            // } 
            // else 
            {

                // No explanation needed, we can request the permission.

                ActivityCompat.requestPermissions(this,
                        new String[]{Manifest.permission.READ_EXTERNAL_STORAGE},
                        READ_EXTERNAL_STORAGE_PERMISSION_ID);
            }
        }
    }    

    private void requestRecordAudioPermission() 
    {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) 
        {
            // Should we show an explanation?
            // if (ActivityCompat.shouldShowRequestPermissionRationale(this, Manifest.permission.READ_EXTERNAL_STORAGE)) 
            // {

            //     // Show an expanation to the user *asynchronously* -- don't block
            //     // this thread waiting for the user's response! After the user
            //     // sees the explanation, try again to request the permission.

            // } 
            // else 
            {

                // No explanation needed, we can request the permission.

                ActivityCompat.requestPermissions(this,
                        new String[]{Manifest.permission.RECORD_AUDIO},
                        RECORD_AUDIO_PERMISSION_ID);
            }
        }
    }    

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestExternalStorageReadPermission();
        requestRecordAudioPermission();

        CommandLine.init(new String[] { "chrome", "--ignore-gpu-blacklist", "--enable-webvr" });

        AwShellResourceProvider.registerResources(this);

        ContentApplication.initCommandLine(this);
        waitForDebuggerIfNeeded();

        ContextUtils.initApplicationContext(getApplicationContext());
        AwBrowserProcess.loadLibrary();
        System.loadLibrary("gvr");
        System.loadLibrary("VRWebGL_GVRMobileSDK");

        if (CommandLine.getInstance().hasSwitch(AwShellSwitches.ENABLE_ATRACE)) {
            Log.e(TAG, "Enabling Android trace.");
            TraceEvent.setATraceEnabled(true);
        }

        layout = new FrameLayout(this); 

        // HACK: Try to identify when the keyboard is shown to be able to hide it and also start sending key events to it.
        layout.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {

                // Always hide the soft keyboard.
                View view = getCurrentFocus();
                if (view != null)
                {
                    InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
                    imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
                }

                // Always hide the soft navigation bar
                setImmersiveSticky();

                // Code to detect if the soft keyboard is shown
                Rect visibleRect = new Rect();
                layout.getWindowVisibleDisplayFrame(visibleRect);
                int screenWidth = layout.getRootView().getWidth();
                if (visibleRect.right < screenWidth && view instanceof WebView)
                {
                    WebView webview = (WebView)view;
                    webview.dispatchEventToVRBrowser("showkeyboard", "{}");
                }
            }
        });        

        setContentView(layout);       

        mAwTestContainerView = createAwTestContainerView();

        layout.addView(mAwTestContainerView);           

        // Ensure fullscreen immersion.
        setImmersiveSticky();
        getWindow()
            .getDecorView()
            .setOnSystemUiVisibilityChangeListener(
                new View.OnSystemUiVisibilityChangeListener() {
                  @Override
                  public void onSystemUiVisibilityChange(int visibility) {
                    if ((visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0 ||
                        (visibility & 
                            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION) == 0) {
                      setImmersiveSticky();
                    }
                  }
                });
        // Initialize GvrLayout and the native renderer.
        gvrLayout = new GvrLayout(this);
        layout.addView(gvrLayout);
        // Add the GLSurfaceView to the GvrLayout.
        surfaceView = new GLSurfaceView(this);
        surfaceView.setZOrderMediaOverlay(true);
        surfaceView.setEGLContextClientVersion(2);
        surfaceView.setEGLConfigChooser(8, 8, 8, 0, 0, 0);
        surfaceView.setPreserveEGLContextOnPause(true);
        surfaceView.setRenderer(this);
        gvrLayout.setPresentationView(surfaceView);
        // Enable scan line racing.
        if (gvrLayout.setAsyncReprojectionEnabled(true)) {
          // Scanline racing decouples the app framerate from the display framerate,
          // allowing immersive interaction even at the throttled clockrates set by
          // sustained performance mode.
          AndroidCompat.setSustainedPerformanceMode(this, true);
        }
        // Enable VR Mode.
        AndroidCompat.setVrModeEnabled(this, true);

        // Prevent screen from dimming/locking.
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

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
        if (config == null)
        {
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
        }
        // If the configuration was provided, use it.
        boolean clearCache = true;
        // The intent, when provided, has more priority than the config asset file.
        String intentURL = getUrlFromIntent(getIntent());
        if (!TextUtils.isEmpty(intentURL)) {
            url = intentURL;
        }
        if (config != null) 
        {
            try
            {
                url = config.has("url") ? config.getString("url") : url;
                clearCache = config.has("clearCache") ? config.getBoolean("clearCache") : clearCache;
            }
            catch(JSONException e) 
            {
                System.out.println("VRWebGL: WARNING: JSONException while trying to retrieve the config properties: " + e.getCause() + " - " + e.getMessage());
            }
        }
        // In case there is not URL, use the default url.
        if (TextUtils.isEmpty(url))
        {
            url = INITIAL_URL;
        }
        mAwTestContainerView.getAwContents().clearCache(clearCache);

        AwContents.setShouldDownloadFavicons();

        audioManager = (AudioManager) getSystemService( Context.AUDIO_SERVICE );

        // Create the native side
        nativePointer = nativeOnCreate( gvrLayout.getGvrApi().getNativeGvrContext() );        

        speechRecognizer = SpeechRecognizer.createSpeechRecognizer(this);
        speechRecognizerIntent = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
        speechRecognizerIntent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_FREE_FORM);
        speechRecognizerIntent.putExtra(RecognizerIntent.EXTRA_CALLING_PACKAGE, this.getPackageName());
        speechRecognizerListener = new SpeechRecognitionListener();
        speechRecognizer.setRecognitionListener(speechRecognizerListener);

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
        gvrLayout.onResume();
        surfaceView.onResume();
        surfaceView.queueEvent(refreshViewerProfileRunnable);

        if (!urlLoaded)
        {
            mAwTestContainerView.getAwContents().loadUrl(url);
            urlLoaded = true;
        }

        surfaceView.setZOrderMediaOverlay(true);
    }

    @Override protected void onPause()
    {
        super.onPause();

        nativeOnPause( nativePointer );
        gvrLayout.onPause();
        surfaceView.onPause();
    }

    @Override protected void onStop()
    {
        super.onStop();
        nativeOnStop( nativePointer );
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        if (mDevToolsServer != null) {
            mDevToolsServer.destroy();
            mDevToolsServer = null;
        }

        // Destruction order is important; shutting down the GvrLayout will detach
        // the GLSurfaceView and stop the GL thread, allowing safe shutdown of
        // native resources from the UI thread.
        gvrLayout.shutdown();

        nativeOnDestroy( nativePointer );
        nativePointer = 0;

        if (speechRecognizer != null)
        {
            speechRecognizer.destroy();
        }
    }

    private AwTestContainerView createAwTestContainerView() {
        AwBrowserProcess.start();
        AwTestContainerView testContainerView = new AwTestContainerView(this, true);
        AwContentsClient awContentsClient = new NullContentsClient() {
            private View mCustomView;
            private boolean jsInjected = false;

            @Override
            public void onPageStarted(String url) {
                // Reset 
                jsInjected = false;
                nativeOnPageStarted();
            }

            @Override
            public void onPageFinished(String url) {
            }

            @Override
            public void onLoadResource(String url) {
            }

            @Override
            public AwWebResourceResponse shouldInterceptRequest(
                    AwContentsClient.AwWebResourceRequest request) {
                // TODO: This is a dirty hack. As it seems that the evalueJavaScript when the main page is loaded does not work because
                // the injected JS is wiped out, we will inject it when another resource is requested. This may or may not work in many 
                // cases depending on when the resource is requested. 
                // Waiting for a better answer from the chromium forums.
                if (!AwShellActivity.this.url.toLowerCase().contains("file://") && !request.url.equals(AwShellActivity.this.url) && !jsInjected)
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

        // for (WebView webview: webviews)
        // {
        //     webview.dispatchTouchEvent(event);
        // }

        return false; // mAwTestContainerView.dispatchTouchEvent(event);
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent event)
    {
        return mAwTestContainerView.dispatchGenericMotionEvent(event);
    }
    
    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) 
    {
        if ( nativePointer != 0 )
        {
            nativeOnSurfaceCreated( nativePointer );
        }
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height)
    {
    }

    @Override
    public void onDrawFrame(GL10 gl) 
    {
        nativeOnDrawFrame(nativePointer);
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

            try {
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
                video.mediaPlayer.prepare();
            } catch (IOException t) {
                Log.e(TAG, "mediaPlayer.prepare failed:" + t.getMessage());
            }
        }        
    }


    public synchronized void newVideo(SurfaceTexture surfaceTexture, int nativeTextureId) 
    {
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

    private synchronized WebView findWebViewBySurfaceTexture(SurfaceTexture surfaceTexture)
    {
        WebView webview = null;
        for (int i = 0; i < webviews.size() && webview == null; i++)
        {
            webview = webviews.get(i);
            if (webview.getSurfaceTexture() != surfaceTexture)
            {
                webview = null;
            }
        }
        return webview;
    }

    private void newWebView(final SurfaceTexture surfaceTexture, final int nativeTextureId)
    {
        runOnUiThread( new Runnable() {
            @Override
            public void run() {
                WebView webview = new WebView(AwShellActivity.this, surfaceTexture, nativeTextureId);
                // webview.surfaceTexture.setOnFrameAvailableListener(this);
                synchronized(AwShellActivity.this)
                {
                    webviews.add(webview);
                }
                addContentView(webview, new ViewGroup.LayoutParams( WebView.WEBVIEW_TEXTURE_WIDTH, WebView.WEBVIEW_TEXTURE_HEIGHT ));
                webview.loadUrl("http://www.google.com");
            }
        });
    }

    public synchronized void deleteWebView(SurfaceTexture surfaceTexture) 
    {
        WebView webview = findWebViewBySurfaceTexture(surfaceTexture);
        ((ViewGroup)webview.getParent()).removeView(webview);
        webviews.remove(webview);
    }

    private void setWebViewSrc(final SurfaceTexture surfaceTexture, final String src)
    {
        runOnUiThread( new Runnable() {
            @Override
            public void run() {
                WebView webview = findWebViewBySurfaceTexture(surfaceTexture);
                if (webview != null)
                {
                    webview.loadUrl(src);
                }
            }
        });
    }

    private void dispatchWebViewTouchEvent(final SurfaceTexture surfaceTexture, final int action, final float x, final float y)
    {
        runOnUiThread( new Runnable() {
            @Override
            public void run() {
                WebView webview = findWebViewBySurfaceTexture(surfaceTexture);
                if (webview != null)
                {
                    float realX = x * WebView.WEBVIEW_TEXTURE_WIDTH;
                    float realY = y * WebView.WEBVIEW_TEXTURE_HEIGHT;
                    int motionEventAction = 0;
                    switch(action)
                    {
                        case WebView.TOUCH_START:
                            motionEventAction = MotionEvent.ACTION_DOWN;
                            break;
                        case WebView.TOUCH_MOVE:
                            motionEventAction = MotionEvent.ACTION_MOVE;
                            break;
                        case WebView.TOUCH_END:
                            motionEventAction = MotionEvent.ACTION_UP;
                            break;
                        default:
                            throw new IllegalArgumentException("The action '" + action + "' to be dispatched as a touch event could not be identified.");
                    }
                    long downTime = android.os.SystemClock.uptimeMillis();
                    long eventTime = android.os.SystemClock.uptimeMillis();
                    MotionEvent motionEvent = MotionEvent.obtain(downTime, eventTime, motionEventAction, realX, realY, 0);
                    webview.dispatchTouchEvent(motionEvent);
                }
            }
        });
    }

    private void dispatchWebViewNavigationEvent(final SurfaceTexture surfaceTexture, final int action)
    {
        runOnUiThread( new Runnable() {
            @Override
            public void run() {
                WebView webview = findWebViewBySurfaceTexture(surfaceTexture);
                if (webview != null)
                {
                    switch(action)
                    {
                        case WebView.NAVIGATION_BACK:
                            webview.goBack();
                            break;
                        case WebView.NAVIGATION_FORWARD:
                            webview.goForward();
                            break;
                        case WebView.NAVIGATION_RELOAD:
                            webview.reload();
                            break;
                        case WebView.NAVIGATION_VOICE_SEARCH:
                            speechRecognizerListener.setWebView(webview);
                            speechRecognizer.startListening(speechRecognizerIntent);
                            break;
                        default:
                            throw new IllegalArgumentException("The action '" + action + "' to be dispatched as a navigation event could not be identified.");
                    }
                }
            }
        });
    }

    private void dispatchWebViewKeyboardEvent(final SurfaceTexture surfaceTexture, final int action, final int keycode)
    {
        runOnUiThread( new Runnable() {
            @Override
            public void run() {
                WebView webview = findWebViewBySurfaceTexture(surfaceTexture);
                if (webview != null)
                {
                    int keyEventAction = 0;
                    switch(action)
                    {
                        case WebView.KEYBOARD_KEY_DOWN:
                            keyEventAction = KeyEvent.ACTION_DOWN;
                            break;
                        case WebView.KEYBOARD_KEY_UP:
                            keyEventAction = KeyEvent.ACTION_UP;
                            break;
                        default:
                            throw new IllegalArgumentException("The action '" + action + "' to be dispatched as a keyboard event could not be identified.");
                    }
                    long downTime = android.os.SystemClock.uptimeMillis();
                    long eventTime = android.os.SystemClock.uptimeMillis();
                    webview.dispatchKeyEvent(new KeyEvent(downTime, eventTime, keyEventAction, keycode, 0));
                }
            }
        });
    }

    private void dispatchWebViewCursorEvent(final SurfaceTexture surfaceTexture, final int action, final float x, final float y)
    {
        runOnUiThread( new Runnable() {
            @Override
            public void run() {
                WebView webview = findWebViewBySurfaceTexture(surfaceTexture);
                if (webview != null)
                {
                    float realX = x * WebView.WEBVIEW_TEXTURE_WIDTH;
                    float realY = y * WebView.WEBVIEW_TEXTURE_HEIGHT;
                    int motionEventAction = 0;
                    switch(action)
                    {
                        case WebView.CURSOR_ENTER:
                            motionEventAction = MotionEvent.ACTION_HOVER_ENTER;
                            break;
                        case WebView.CURSOR_MOVE:
                            motionEventAction = MotionEvent.ACTION_HOVER_MOVE;
                            break;
                        case WebView.CURSOR_EXIT:
                            motionEventAction = MotionEvent.ACTION_HOVER_EXIT;
                            break;
                        default:
                            throw new IllegalArgumentException("The action '" + action + "' to be dispatched as a touch event could not be identified.");
                    }
                    long downTime = android.os.SystemClock.uptimeMillis();
                    long eventTime = android.os.SystemClock.uptimeMillis();
                    webview.pointerProperties[0].id = 0;
                    webview.pointerProperties[0].toolType = MotionEvent.TOOL_TYPE_MOUSE;
                    webview.pointerCoords[0].x = realX;
                    webview.pointerCoords[0].y = realY;
                    MotionEvent motionEvent = MotionEvent.obtain(downTime, eventTime, motionEventAction, 1, webview.pointerProperties, webview.pointerCoords, 0, 0, 1.0f, 1.0f, 0, 0, android.view.InputDevice.SOURCE_CLASS_POINTER, 0);
                    webview.dispatchGenericMotionEvent(motionEvent);
                }
            }
        });
    }

    // Native calls
    
    // Activity lifecycle
    private native long nativeOnCreate( long nativeGvrContext );
    private native void nativeOnStart( long nativePointer );
    private native void nativeOnResume( long nativePointer );
    private native void nativeOnPause( long nativePointer );
    private native void nativeOnStop( long nativePointer );
    private native void nativeOnDestroy( long nativePointer );

    private native void nativeOnPageStarted();

    // Surface lifecycle
    public native void nativeOnSurfaceCreated( long nativePointer);
    public native void nativeOnDrawFrame( long nativePointer );

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
