/**
 * eBrowser Extension - Content Script
 * Injected into all pages for privacy protection and feature injection
 */

'use strict';

(function () {
  // Prevent double-injection
  if (window.__eBrowserInjected) return;
  window.__eBrowserInjected = true;

  // ─── GPC (Global Privacy Control) ──────────────────────────────────────────
  Object.defineProperty(navigator, 'globalPrivacyControl', {
    value: true,
    writable: false,
    enumerable: true,
    configurable: false,
  });

  // ─── DNT (Do Not Track) ────────────────────────────────────────────────────
  // Note: DNT is deprecated but still respected by some sites
  // The browser already sets this header; this is for JS-based detection

  // ─── Tracker blocking (JS-based) ───────────────────────────────────────────
  const TRACKER_PATTERNS = [
    /google-analytics\.com/,
    /googletagmanager\.com/,
    /doubleclick\.net/,
    /facebook\.net\/en_US\/fbevents/,
    /connect\.facebook\.net/,
    /analytics\.twitter\.com/,
    /static\.ads-twitter\.com/,
    /bat\.bing\.com/,
    /scorecardresearch\.com/,
    /quantserve\.com/,
    /hotjar\.com/,
    /mixpanel\.com/,
    /segment\.io/,
    /amplitude\.com/,
  ];

  // Override XMLHttpRequest to block trackers
  const OriginalXHR = window.XMLHttpRequest;
  window.XMLHttpRequest = function () {
    const xhr = new OriginalXHR();
    const originalOpen = xhr.open.bind(xhr);
    xhr.open = function (method, url, ...args) {
      if (TRACKER_PATTERNS.some(p => p.test(url))) {
        console.debug('[eBrowser] Blocked tracker XHR:', url);
        chrome.runtime.sendMessage({ type: 'BLOCK_TRACKER', url });
        return;
      }
      return originalOpen(method, url, ...args);
    };
    return xhr;
  };

  // ─── Reading Mode detection ─────────────────────────────────────────────────
  function getReadabilityScore() {
    const paragraphs = document.querySelectorAll('p');
    const textLength = Array.from(paragraphs).reduce((sum, p) => sum + p.textContent.length, 0);
    return textLength;
  }

  // ─── Page metadata extraction ───────────────────────────────────────────────
  function extractMetadata() {
    const title = document.title;
    const description = document.querySelector('meta[name="description"]')?.content || '';
    const ogImage = document.querySelector('meta[property="og:image"]')?.content || '';
    const canonical = document.querySelector('link[rel="canonical"]')?.href || window.location.href;
    const readabilityScore = getReadabilityScore();
    return { title, description, ogImage, canonical, readabilityScore };
  }

  // ─── Message listener ───────────────────────────────────────────────────────
  chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
    switch (message.type) {
      case 'GET_PAGE_METADATA':
        sendResponse(extractMetadata());
        break;

      case 'ENABLE_READING_MODE': {
        enableReadingMode();
        sendResponse({ success: true });
        break;
      }

      case 'SCROLL_TO':
        window.scrollTo({ top: message.y, behavior: 'smooth' });
        sendResponse({ success: true });
        break;

      case 'GET_SELECTED_TEXT':
        sendResponse({ text: window.getSelection()?.toString() || '' });
        break;
    }
  });

  // ─── Reading Mode ───────────────────────────────────────────────────────────
  function enableReadingMode() {
    const article = document.querySelector('article') ||
      document.querySelector('[role="main"]') ||
      document.querySelector('main') ||
      document.body;

    const content = article.innerHTML;
    const title = document.title;

    document.body.innerHTML = `
      <div style="
        max-width: 680px;
        margin: 0 auto;
        padding: 40px 20px;
        font-family: Georgia, 'Times New Roman', serif;
        font-size: 20px;
        line-height: 1.8;
        color: #1a1a1a;
        background: #fff;
      ">
        <h1 style="font-size: 2em; margin-bottom: 0.5em; line-height: 1.3;">${title}</h1>
        <div>${content}</div>
      </div>
    `;
  }

  // ─── Keyboard shortcut: Alt+R for reading mode ──────────────────────────────
  document.addEventListener('keydown', (e) => {
    if (e.altKey && e.key === 'r') {
      enableReadingMode();
    }
  });

})();
