/**
 * eBrowser Extension - Background Service Worker
 * Handles tab management, privacy features, and sync
 */

'use strict';

// ─── Installation & Update ────────────────────────────────────────────────────

chrome.runtime.onInstalled.addListener(async ({ reason }) => {
  if (reason === 'install') {
    console.log('[eBrowser] Extension installed');
    await chrome.storage.local.set({
      settings: {
        blockTrackers: true,
        blockAds: false,
        enableHTTPSOnly: false,
        enableDNT: true,
        enableGPC: true,
        theme: 'system',
        searchEngine: 'google',
      },
      stats: { trackersBlocked: 0, adsBlocked: 0, httpsUpgrades: 0 },
    });
    // Open welcome page
    chrome.tabs.create({ url: chrome.runtime.getURL('newtab.html') });
  } else if (reason === 'update') {
    console.log('[eBrowser] Extension updated');
  }

  // Set up context menus
  setupContextMenus();
});

// ─── Context Menus ────────────────────────────────────────────────────────────

function setupContextMenus() {
  chrome.contextMenus.removeAll(() => {
    chrome.contextMenus.create({
      id: 'ebrowser-root',
      title: 'eBrowser',
      contexts: ['all'],
    });
    chrome.contextMenus.create({
      id: 'save-bookmark',
      parentId: 'ebrowser-root',
      title: 'Save to Bookmarks',
      contexts: ['page', 'link'],
    });
    chrome.contextMenus.create({
      id: 'add-reading-list',
      parentId: 'ebrowser-root',
      title: 'Add to Reading List',
      contexts: ['page'],
    });
    chrome.contextMenus.create({
      id: 'search-selected',
      parentId: 'ebrowser-root',
      title: 'Search for "%s"',
      contexts: ['selection'],
    });
    chrome.contextMenus.create({
      id: 'open-incognito',
      parentId: 'ebrowser-root',
      title: 'Open Link in Incognito',
      contexts: ['link'],
    });
    chrome.contextMenus.create({
      id: 'separator-1',
      parentId: 'ebrowser-root',
      type: 'separator',
      contexts: ['all'],
    });
    chrome.contextMenus.create({
      id: 'toggle-privacy',
      parentId: 'ebrowser-root',
      title: 'Toggle Privacy Mode',
      contexts: ['all'],
    });
  });
}

chrome.contextMenus.onClicked.addListener(async (info, tab) => {
  switch (info.menuItemId) {
    case 'save-bookmark': {
      const url = info.linkUrl || info.pageUrl;
      const title = tab?.title || url;
      await chrome.bookmarks.create({ title, url });
      showNotification('Bookmark Saved', `"${title}" added to bookmarks`);
      break;
    }
    case 'search-selected': {
      const query = encodeURIComponent(info.selectionText || '');
      chrome.tabs.create({ url: `https://www.google.com/search?q=${query}` });
      break;
    }
    case 'open-incognito': {
      chrome.windows.create({ url: info.linkUrl, incognito: true });
      break;
    }
    case 'toggle-privacy': {
      const { settings } = await chrome.storage.local.get('settings');
      await chrome.storage.local.set({
        settings: { ...settings, blockTrackers: !settings.blockTrackers },
      });
      showNotification('Privacy Mode', settings.blockTrackers ? 'Privacy mode disabled' : 'Privacy mode enabled');
      break;
    }
  }
});

// ─── Tab Events ───────────────────────────────────────────────────────────────

chrome.tabs.onUpdated.addListener(async (tabId, changeInfo, tab) => {
  if (changeInfo.status === 'complete' && tab.url) {
    // Record history
    try {
      const history = await chrome.storage.local.get('history');
      const entries = history.history || [];
      const existing = entries.findIndex(e => e.url === tab.url);
      if (existing >= 0) {
        entries[existing].visitCount++;
        entries[existing].visitedAt = Date.now();
        entries[existing].title = tab.title;
      } else {
        entries.unshift({ url: tab.url, title: tab.title, favicon: tab.favIconUrl, visitedAt: Date.now(), visitCount: 1 });
        if (entries.length > 1000) entries.pop();
      }
      await chrome.storage.local.set({ history: entries });
    } catch (e) { /* ignore */ }

    // HTTPS upgrade tracking
    if (tab.url.startsWith('https://')) {
      const { stats } = await chrome.storage.local.get('stats');
      if (stats) {
        stats.httpsUpgrades = (stats.httpsUpgrades || 0) + 1;
        await chrome.storage.local.set({ stats });
      }
    }
  }
});

// ─── Commands ─────────────────────────────────────────────────────────────────

chrome.commands.onCommand.addListener(async (command) => {
  switch (command) {
    case 'toggle-privacy': {
      const { settings } = await chrome.storage.local.get('settings');
      await chrome.storage.local.set({
        settings: { ...settings, blockTrackers: !settings.blockTrackers },
      });
      break;
    }
    case 'new-incognito': {
      chrome.windows.create({ url: 'about:newtab', incognito: true });
      break;
    }
  }
});

// ─── Message Handling ─────────────────────────────────────────────────────────

chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  switch (message.type) {
    case 'GET_SETTINGS':
      chrome.storage.local.get('settings').then(({ settings }) => sendResponse({ settings }));
      return true;

    case 'UPDATE_SETTINGS':
      chrome.storage.local.get('settings').then(async ({ settings }) => {
        const updated = { ...settings, ...message.settings };
        await chrome.storage.local.set({ settings: updated });
        sendResponse({ success: true });
      });
      return true;

    case 'GET_STATS':
      chrome.storage.local.get('stats').then(({ stats }) => sendResponse({ stats }));
      return true;

    case 'GET_HISTORY':
      chrome.storage.local.get('history').then(({ history }) => sendResponse({ history: history || [] }));
      return true;

    case 'BLOCK_TRACKER':
      chrome.storage.local.get('stats').then(async ({ stats }) => {
        if (stats) {
          stats.trackersBlocked = (stats.trackersBlocked || 0) + 1;
          await chrome.storage.local.set({ stats });
        }
      });
      break;
  }
});

// ─── Alarms (periodic tasks) ──────────────────────────────────────────────────

chrome.alarms.create('cleanup-history', { periodInMinutes: 60 * 24 });

chrome.alarms.onAlarm.addListener(async (alarm) => {
  if (alarm.name === 'cleanup-history') {
    const { history } = await chrome.storage.local.get('history');
    if (history && history.length > 500) {
      await chrome.storage.local.set({ history: history.slice(0, 500) });
    }
  }
});

// ─── Helpers ──────────────────────────────────────────────────────────────────

function showNotification(title, message) {
  chrome.notifications.create({
    type: 'basic',
    iconUrl: chrome.runtime.getURL('icons/icon48.png'),
    title: `eBrowser: ${title}`,
    message,
  });
}
