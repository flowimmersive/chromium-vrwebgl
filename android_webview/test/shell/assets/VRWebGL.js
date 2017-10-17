(function() {
	if (typeof(window.VRWebGLRenderingContext) !== "undefined") {

		var makeOriginalWebGLCalls = false;

        // FULL WEBVR 1.0 API
        function notifyVRDisplayPresentChangeEvent(vrDisplay) {
            var event = new CustomEvent('vrdisplaypresentchange', {detail: {vrdisplay: self}});
            window.dispatchEvent(event);
            if (typeof(window.onvrdisplaypresentchange) === "function") {
                window.onvrdisplaypresentchange(event);
            }
        }

        var nextDisplayId = 1000;

        VRDisplay = function() {
            var _layers = null;
            var _rigthEyeParameters = new VREyeParameters();
            var _leftEyeParameters = new VREyeParameters();
            var _pose = new VRPose();
            _pose.orientation = new Float32Array(4);

            this.isConnected = false;
            this.isPresenting = false;
            this.capabilities = new VRDisplayCapabilities();
            this.capabilities.hasOrientation = true;
            this.capabilities.canPresent = true;
            this.capabilities.maxLayers = 1;
            // this.stageParameters = null; // OculusMobileSDK (Gear VR) does not support room scale VR yet, this attribute is optional.
            this.getEyeParameters = function(eye) {
                var eyeParameters = null;
                if (vrWebGLRenderingContexts.length > 0) {
                    eyeParameters = vrWebGLRenderingContexts[0].getEyeParameters(eye);
                }
                if (eyeParameters !== null && eye === 'left') {
                    eyeParameters.offset = -eyeParameters.offset;
                }
                return eyeParameters;
            };
            this.displayId = nextDisplayId++;
            this.displayName = 'VRWebGL Oculus Mobile deviceName';
            this.getPose = function() {
                var pose = null;
                if (vrWebGLRenderingContexts.length > 0) {
                    pose = vrWebGLRenderingContexts[0].getPose();
                }
                return pose;
            };
            this.getImmediatePose = function() {
                return getPose();
            };
            this.resetPose = function() {
                // TODO: Make a call to the native extension to reset the pose.
            };
            this.depthNear = 0.01;
            this.depthFar = 10000.0;
            this.requestAnimationFrame = function(callback) {
                return window.requestAnimationFrame(callback);
            };
            this.cancelAnimationFrame = function(handle) {
                return window.cancelAnimationFrame(handle);
            };
            this.requestPresent = function(layers) {
                var self = this;
                return new Promise(function(resolve, reject) {
                    self.isPresenting = true;
                    notifyVRDisplayPresentChangeEvent(self);
                    _layers = layers;
                    resolve();
                });
            };
            this.exitPresent = function() {
                var self = this;
                return new Promise(function(resolve, reject) {
                    self.isPresenting = false;
                    resolve();
                });
            };
            this.getLayers = function() {
                return _layers;
            };
            this.submitFrame = function(pose) {
                // TODO: Learn fom the WebVR Polyfill how to make the barrel distortion.
            };
            return this;
        };

        VRLayer = function() {
            this.source = null;
            this.leftBounds = [];
            this.rightBounds = [];
            return this;
        };

        VRDisplayCapabilities = function() {
            this.hasPosition = false;
            this.hasOrientation = false;
            this.hasExternalDisplay = false;
            this.canPresent = false;
            this.maxLayers = 0;
            return this;
        };

        VREye = {
            left: "left",
            right: "right"
        };

        VRFieldOfView = function() {
            this.upDegrees = 0;
            this.rightDegrees = 0;
            this.downDegrees = 0;
            this.leftDegrees = 0;
            return this;
        };

        VRPose = function() {
            this.timeStamp = 0;
            this.position = null;
            this.linearVelocity = null;
            this.linearAcceleration = null;
            this.orientation = null;
            this.angularVelocity = null;
            this.angularAcceleration = null;
            return this;
        };

        VREyeParameters = function() {
            this.offset = 0;
            this.fieldOfView = new VRFieldOfView();
            this.renderWidth = 0;
            this.renderHeight = 0;
            return this;
        };

        VRStageParameters = function() {
            this.sittingToStandingTransform = null;
            this.sizeX = 0;
            this.sizeZ = 0;
            return this;
        };

        // The VR displayes
        var displays = [ new VRDisplay() ];
        // The promise resolvers for those promises created before the start event is received === devices are created.
        var resolvers = [];

        navigator.getVRDisplays = function() {
            return new Promise(
                function(resolve, reject) {
                    resolve(displays);
                });
        };

		var originalNavigatorGetGamepads = navigator.getGamepads;
		navigator.getGamepads = function() {
			// var argumentsArray = Array.prototype.slice.apply(arguments);
			var argumentsArray = new Array(arguments.length);
			for (var i = 0; i < arguments.length; i++) {
				argumentsArray[i] = arguments[i];
			}
			var originalGamepads = originalNavigatorGetGamepads.apply(navigator, argumentsArray);

			// TODO: Maybe even remove/replace the DD controller if it exists.
			if (vrWebGLRenderingContexts.length > 0) {
				var gamepad = vrWebGLRenderingContexts[0].getGamepad();
				if (gamepad) {
					// The originalGamepads cannot be modified so we will create a proper array :).
					var gamepads = new Array(originalGamepads.length);
					var freeIndex = -1;
					for (var i = 0; i < originalGamepads.length; i++) {
						gamepads[i] = originalGamepads[i];
						if (freeIndex === -1 && gamepads[i] === null) {
							// Even though we have found a free index, keep going to copy all the originalGamepads to the gamepads array.
							freeIndex = i;
						}
					}
					// If all the slots were taken, add the gamepad to the end.
					if (freeIndex < 0 ) {
						gamepads.push(gamepad);
					}
					else {
						gamepads[freeIndex] = gamepad;
					}
					// As we are always returning the original gamepads variable, make sure that it points to the new structure.
					originalGamepads = gamepads;
				}
			}
			return originalGamepads;
		}

		// Store the original requestAnimationFrame function as we want to inject ours.
		// We need to have control over the requestAnimationFrame to identify the webGL calls that should be performes in the native render loop/function.
		var originalRequestAnimationFrame = window.requestAnimationFrame;

		// We will store all the request animation frame callbacks in a queue, the same way the native side does it.
		// If, for some reason, more than one callback is set before the previous one is processed, we need to make sure that
		// we hold on to the previous functions too to keep their context/closures.
		var requestAnimationFrameCallbacks = [];

		var vrWebGLRenderingContexts = [];
		var vrWebGLVideos = [];
		var vrWebGLWebViews = [];
		var vrWebGLSpeechRecognitions = [];
		var vrWebGLArrays = [
			vrWebGLVideos,
			vrWebGLWebViews,
			vrWebGLSpeechRecognitions ];

		function vrWebGLRequestAnimationFrame() {
			// Update the gamepad if the VRWebGLRenderingContext has one by simply retrieving it
			if (vrWebGLRenderingContexts.length > 0) {
				vrWebGLRenderingContexts[0].getGamepad();
			}

			// Mark the beginning of a frame
			for (var i = 0; i < vrWebGLRenderingContexts.length; i++) {
				vrWebGLRenderingContexts[i].startFrame();
			}

			// Call the original raf callbacks
			// var argumentsArray = Array.prototype.slice.apply(arguments);
			var argumentsArray = new Array(arguments.length);
			for (var i = 0; i < arguments.length; i++) {
				argumentsArray[i] = arguments[i];
			}

			requestAnimationFrameCallbacks[0].apply(this, argumentsArray);
			requestAnimationFrameCallbacks.splice(0, 1);

			// Mark the end of a frame
			for (var i = 0; i < vrWebGLRenderingContexts.length; i++) {
				vrWebGLRenderingContexts[i].endFrame();
			}
		}

		window.requestAnimationFrame = function(callback) {
			requestAnimationFrameCallbacks.push(callback);
			originalRequestAnimationFrame.call(this, vrWebGLRequestAnimationFrame);
		};

		// Replace the original WebGLRenderingContext for the VRWebGLRenderingContext
		var originalWebGLRenderingContext = window.WebGLRenderingContext;
		window.WebGLRenderingContext = window.VRWebGLRenderingContext;

		// Store the original VRWebGL texImage2D function prototype as we will slightly change it but still call it.
        /*
        var originalVRWebGLTexImage2D = window.VRWebGLRenderingContext.prototype.texImage2D;
		window.VRWebGLRenderingContext.prototype.texImage2D = function() {
			// var argumentsArray = Array.prototype.slice.apply(arguments);
			var argumentsArray = new Array(arguments.length);
			for (var i = 0; i < arguments.length; i++) {
				argumentsArray[i] = arguments[i];
			}

			var result = undefined;
			// These are all the possible call options according to the WebGL spec
			// 1.- void gl.texImage2D(target, level, internalformat, width, height, border, format, type, ArrayBufferView? pixels);
			// 2.- void gl.texImage2D(target, level, internalformat, format, type, ImageData? pixels);
			// 3.- void gl.texImage2D(target, level, internalformat, format, type, HTMLImageElement? pixels);
			// 4.- void gl.texImage2D(target, level, internalformat, format, type, HTMLCanvasElement? pixels);
			// 5.- void gl.texImage2D(target, level, internalformat, format, type, HTMLVideoElement? pixels);
			if (argumentsArray.length === 6) {
				if (argumentsArray[5] instanceof HTMLCanvasElement) {
					// As the current implementation of the VRWebGLRenderingContext does not have support for HTMLCanvasElement in the texImage2D overloads, convert the canvas to something else that is supported.

                    var canvas = argumentsArray[5]
					var canvasInBase64 = canvas.toDataURL();

					// OPTION 1: Convert the canvas to an array buffer.
					var byteString = atob(canvasInBase64.split(',')[1]);
					var ab = new ArrayBuffer(byteString.length);
					var ia = new Uint8Array(ab);
                    console.log(byteString.length);
					for (var i = 0; i < byteString.length; i++) {
					  ia[i] = byteString.charCodeAt(i);
					}
					var png = new PNG(ia);
					var pixels = png.decodePixels();
                    argumentsArray[3] = canvas.width;
                    argumentsArray[4] = canvas.height;
                    argumentsArray[5] = 0; // border
					argumentsArray[6] = argumentsArray[3]; // format
					argumentsArray[7] = argumentsArray[4]; // type
					argumentsArray[8] = pixels;
					return originalVRWebGLTexImage2D.apply(this, argumentsArray);

					// OPTION 2: Convert the canvas to an image.
					// This option worked in Chromium 54 but seems to be broken in 57 and above as the loading of the image
					// seems to be asynchronous and we need it to be synchronous. That is why OPTION 1 has been implemented.
					// var image = new Image();
					// image.src = canvasInBase64;
					// argumentsArray[5] = image;
				}

				// TODO: Still 2 calls are not being handled: the ones that pass these parameters. ImageData and HTMLVideoElement
			}
			return originalVRWebGLTexImage2D.apply(this, argumentsArray);
		}
        */

		// Store the original HTMLCanvasElement getContext function as we want to inject ours.
		var originalHTMLCanvasElementPrototypeGetContextFunction = HTMLCanvasElement.prototype.getContext;
		// Replace the HTMLCanvasElement getContext function with out own version
		HTMLCanvasElement.prototype.getContext = function() {
			// var argumentsArray = Array.prototype.slice.apply(arguments);
			var argumentsArray = new Array(arguments.length);
			for (var i = 0; i < arguments.length; i++) {
				argumentsArray[i] = arguments[i];
			}

			if (typeof(argumentsArray[0]) === "string" && (argumentsArray[0] === "webgl" || argumentsArray[0] === "experimental-webgl")) {
				if (vrWebGLRenderingContexts.length == 0) {
					vrWebGLRenderingContexts.push(new VRWebGLRenderingContext());
					vrWebGLRenderingContexts[0].canvas = this;
				}
				return vrWebGLRenderingContexts[0];
			}
			else {
				return originalHTMLCanvasElementPrototypeGetContextFunction.apply(this, argumentsArray);
			}
		};

		function addEventHandlingToObject(object) {
			object.listeners = { };
			object.addEventListener = function(eventType, callback) {
				if (!callback) return this;
				var listeners = this.listeners[eventType];
				if (!listeners) {
					listeners = [];
					this.listeners[eventType] = listeners;
				}
				if (listeners.indexOf(callback) < 0) {
					listeners.push(callback);
				}
				return this;
			};
			object.removeEventListener = function(eventType, callback) {
				if (!callback) return this;
				var listeners = this.listeners[eventType];
				if (listeners) {
					var i = listeners.indexOf(callback);
					if (i >= 0) {
						this.listeners[eventType] = listeners.splice(i, 1);
					}
				}
				return this;
			};
			object.callEventListeners = function(eventType, event) {
				if (!event) event = { target : this };
				if (!event.target) event.target = this;
				event.type = eventType;
				var onEventType = 'on' + eventType;
				if (typeof(this[onEventType]) === 'function') {
					this[onEventType](event)
				}
				var listeners = this.listeners[eventType];
				if (listeners) {
					for (var i = 0; i < listeners.length; i++) {
						var typeofListener = typeof(listeners[i]);
						if (typeofListener === "object") {
							listeners[i].handleEvent(event);
						}
						else if (typeofListener === "function") {
							listeners[i](event);
						}
					}
				}
				return this;
			};
		}

		// Setup to be able to create VRWebGLVideo or VRWebGLWebView using
		// document.createElement("video"/"webview").
		// Replace the original document.createElement function with our own to be able to create the correct video element for VRWebGL
		var originalDocumentCreateElementFunction = document.createElement;
		document.createElement = function() {
			// var argumentsArray = Array.prototype.slice.apply(arguments);
			var argumentsArray = new Array(arguments.length);
			for (var i = 0; i < arguments.length; i++) {
				argumentsArray[i] = arguments[i];
			}

			// VRWebGLVideo instance can only be created if a URL parameter
			// is passed.
			if (typeof(argumentsArray[0]) === "string" &&
					argumentsArray[0] === "video") {
				var vrWebGLVideo = new VRWebGLVideo();
				vrWebGLVideos.push(vrWebGLVideo);
				addEventHandlingToObject(vrWebGLVideo);
				return vrWebGLVideo;
			}
			else if (typeof(argumentsArray[0]) === "string" &&
					argumentsArray[0] === "webview") {
				var vrWebGLWebView = new VRWebGLWebView();
				vrWebGLWebViews.push(vrWebGLWebView);
				addEventHandlingToObject(vrWebGLWebView);
				return vrWebGLWebView;
			}
			else {
				return originalDocumentCreateElementFunction.apply(this, argumentsArray);
			}
		};

		// A new function in the document to be able to make sure that
		// an element is correctly destroyed. In our case, VRWebGLVideo,
		// VRWebGLWebView and even VRWebGLSpeechRecognition instances
		// that are kept in their corresponding arrays.
		document.deleteElement = function(obj) {
			// Look for the video or the webview to correctly remove it from
			// the corresponding array.
			var found = false;
			for (var j = 0; !found && j < vrWebGLArrays.length; j++) {
				var vrWebGLArray = vrWebGLArrays[j];
				for (var i =0; !found && i < vrWebGLArray.length; i++) {
					if (vrWebGLArray[i] === obj) {
						vrWebGLArray.splice(i, 1);
						found = true;
					}
				}
			}
			return this;
		};

		// Return any of the instances in the arrays by id.
		function findById(index, id) {
			if (index < 0 || index >= vrWebGLArrays.length)
				throw "ERROR: The provided index '" + index + "' is out of scope.";
			var obj = null;
			var vrWebGLArray = vrWebGLArrays[index];
			for (var i =0; !obj && i < vrWebGLArray.length; i++) {
				obj = vrWebGLArray[i];
				if (obj.id !== id) {
					obj = null;
				}
			}
			return obj;
		}

		// Setup the webkitSpeechRecognition API to be able to use the
		// underlying VRWebGLSpeechRecognition instances, but only, if
		// requested by a URL parameter. Add the created instance to be able to
		// use them afterward when events are dispatched.
		if (location.search.includes("vrwebglspeechrecognition")) {
			var originalWindowWebkitSpeechRecognition =
				window.webkitSpeechRecognition;
			window.webkitSpeechRecognition = function() {
				var vrWebGLSpeechRecognition = new VRWebGLSpeechRecognition();
				addEventHandlingToObject(vrWebGLSpeechRecognition);
				vrWebGLSpeechRecognitions.push(vrWebGLSpeechRecognition);
				return vrWebGLSpeechRecognition;
			}
		}

		// Expose a way so the native side can dispatch events to the event handling objects in the arrays.
		window.vrwebgl = {
			dispatchEvent: function(index, id, eventName, event) {
				var obj = findById(index, id);
				if (obj) {
					// NOTE: Very specific handling for the video ended event.
					if (obj instanceof VRWebGLVideo) {
						if (eventName === "ended") {
							obj.ended = true;
						}
						else if (eventName === "canplaythrough") {
							obj.readyState = 2;
						}
					}
					obj.callEventListeners(eventName, event);
				}
				return this;
			}
		};

		console.log("localStorage in Window: "+ window.localStorage);
		//VRWebGL doesn't have localStorage, so use this polyfill
        if ( ! window.localStorage ) {
            console.log("loading localstorage pollyfill into storage");
            window.storage = {
                _data       : {},
                setItem     : function(id, val) { return this._data[id] = String(val); },
                getItem     : function(id) { return this._data.hasOwnProperty(id) ? this._data[id] : undefined; },
                removeItem  : function(id) { return delete this._data[id]; },
                clear       : function() { return this._data = {}; }
            };
            console.log("after: " + window.storage);
        }

	}
})();
