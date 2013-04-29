/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
"use strict";

let { classes: Cc, interfaces: Ci, results: Cr, utils: Cu }  = Components;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Geometry.jsm");
Cu.import("resource://gre/modules/AddonManager.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");

let HTMLSelectElement = Ci.nsIDOMHTMLSelectElement;
let HTMLLabelElement = Ci.nsIDOMHTMLLabelElement;
let HTMLIFrameElement = Ci.nsIDOMHTMLIFrameElement;
let HTMLFrameElement = Ci.nsIDOMHTMLFrameElement;

XPCOMUtils.defineLazyServiceGetter(this, "DOMUtils",
  "@mozilla.org/inspector/dom-utils;1", "inIDOMUtils");

XPCOMUtils.defineLazyServiceGetter(Services, "embedlite",
                                    "@mozilla.org/embedlite-app-service;1",
                                    "nsIEmbedAppService");

const kStateActive = 0x00000001; // :active pseudoclass for elements

dump("###################################### TestHelper.js loaded\n");

var globalObject = null;

function TestHelper() {
  this._init();
}

TestHelper.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  _fastFind: null,
  _init: function()
  {
    dump("Init Called:" + this + "\n");
    // addEventListener("touchend", this, false);
    // Services.obs.addObserver(this, "before-first-paint", true);
    addMessageListener("embedtest:getelementprop", this);
    addMessageListener("embedtest:getelementinner", this);
  },

  observe: function(aSubject, aTopic, data) {
    // Ignore notifications not about our document.
    dump("observe topic:" + aTopic + "\n");
  },

  receiveMessage: function receiveMessage(aMessage) {
    switch (aMessage.name) {
      case "embedtest:getelementprop": {
        let element = content.document.getElementById(aMessage.json.name);
        sendAsyncMessage("testembed:elementpropvalue", {value: element.value});
        break;
      }
      case "embedtest:getelementinner": {
        let element = content.document.getElementById(aMessage.json.name);
        sendAsyncMessage("testembed:elementinnervalue", {value: element.innerHTML});
        break;
      }
      default: {
        break;
      }
    }
  },
};

globalObject = new TestHelper();

