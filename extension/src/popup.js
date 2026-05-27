'use strict';

document.addEventListener('DOMContentLoaded', async () => {
  // Load settings
  const { settings } = await chrome.storage.local.get('settings');
  if (settings) {
    document.getElementById('block-trackers').checked = settings.blockTrackers ?? true;
    document.getElementById('block-ads').checked = settings.blockAds ?? false;
    document.getElementById('https-only').checked = settings.enableHTTPSOnly ?? false;
    document.getElementById('enable-doh').checked = settings.enableDoH ?? true;
  }

  // Load stats
  const { stats } = await chrome.storage.local.get('stats');
  if (stats) {
    document.getElementById('trackers-count').textContent = formatNumber(stats.trackersBlocked || 0);
    document.getElementById('ads-count').textContent = formatNumber(stats.adsBlocked || 0);
    document.getElementById('https-count').textContent = formatNumber(stats.httpsUpgrades || 0);
  }

  // Apply dark mode
  if (settings?.theme === 'dark' || (settings?.theme === 'system' && window.matchMedia('(prefers-color-scheme: dark)').matches)) {
    document.body.classList.add('dark');
  }

  // Toggle handlers
  const toggles = [
    { id: 'block-trackers', key: 'blockTrackers' },
    { id: 'block-ads', key: 'blockAds' },
    { id: 'https-only', key: 'enableHTTPSOnly' },
    { id: 'enable-doh', key: 'enableDoH' },
  ];

  for (const { id, key } of toggles) {
    document.getElementById(id).addEventListener('change', async (e) => {
      const { settings } = await chrome.storage.local.get('settings');
      await chrome.storage.local.set({
        settings: { ...settings, [key]: e.target.checked },
      });
    });
  }

  // Quick action buttons
  document.getElementById('open-history').addEventListener('click', (e) => {
    e.preventDefault();
    chrome.tabs.create({ url: 'chrome://history' });
    window.close();
  });

  document.getElementById('open-bookmarks').addEventListener('click', (e) => {
    e.preventDefault();
    chrome.tabs.create({ url: 'chrome://bookmarks' });
    window.close();
  });

  document.getElementById('open-downloads').addEventListener('click', (e) => {
    e.preventDefault();
    chrome.tabs.create({ url: 'chrome://downloads' });
    window.close();
  });

  document.getElementById('open-settings').addEventListener('click', (e) => {
    e.preventDefault();
    chrome.runtime.openOptionsPage();
    window.close();
  });
});

function formatNumber(n) {
  if (n >= 1000000) return (n / 1000000).toFixed(1) + 'M';
  if (n >= 1000) return (n / 1000).toFixed(1) + 'K';
  return n.toString();
}
