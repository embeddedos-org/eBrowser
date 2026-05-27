import { create } from 'zustand';
import { immer } from 'zustand/middleware/immer';
import { persist, createJSONStorage } from 'zustand/middleware';

export type TabStatus = 'loading' | 'complete' | 'error' | 'idle';
export type SecurityLevel = 'secure' | 'warning' | 'danger' | 'local' | 'extension';
export type ThemeMode = 'light' | 'dark' | 'system';
export type PrivacyMode = 'normal' | 'incognito' | 'tor';
export type SearchEngine = 'google' | 'bing' | 'duckduckgo' | 'brave' | 'ecosia' | 'startpage' | 'custom';
export type SidebarPanel = 'bookmarks' | 'history' | 'downloads' | 'notes' | 'reading-list' | 'extensions' | null;

export interface Tab {
  id: string;
  url: string;
  displayUrl: string;
  title: string;
  favicon?: string;
  status: TabStatus;
  securityLevel: SecurityLevel;
  isLoading: boolean;
  canGoBack: boolean;
  canGoForward: boolean;
  isIncognito: boolean;
  isMuted: boolean;
  isPinned: boolean;
  isBookmarked: boolean;
  scrollPosition?: number;
  zoom: number;
  history: string[];
  historyIndex: number;
  createdAt: Date;
  lastActiveAt: Date;
  screenshot?: string;
  readingMode: boolean;
  splitView?: string; // tab id of split partner
}

export interface SearchEngineConfig {
  id: SearchEngine;
  name: string;
  url: string;
  suggestUrl?: string;
  icon: string;
  customUrl?: string;
}

export interface Settings {
  theme: ThemeMode;
  language: string;
  searchEngine: SearchEngine;
  customSearchUrl?: string;
  homepage: string;
  newTabPage: 'default' | 'blank' | 'custom';
  customNewTabUrl?: string;
  showBookmarksBar: boolean;
  showStatusBar: boolean;
  enableAnimations: boolean;
  fontSize: 'small' | 'medium' | 'large' | 'xlarge';
  defaultZoom: number;
  enableSpellCheck: boolean;
  enableAutoFill: boolean;
  enablePasswordManager: boolean;
  enableSyncHistory: boolean;
  enableSyncBookmarks: boolean;
  enableSyncSettings: boolean;
  privacyMode: PrivacyMode;
  blockTrackers: boolean;
  blockAds: boolean;
  blockCookies: 'none' | 'third-party' | 'all';
  enableDoH: boolean;
  dohProvider: string;
  enableHTTPSOnly: boolean;
  enableAntiFingerprint: boolean;
  enableGPC: boolean;
  enableDNT: boolean;
  sendCrashReports: boolean;
  sendUsageStats: boolean;
  enableHardwareAcceleration: boolean;
  enablePrefetch: boolean;
  enablePrerender: boolean;
  downloadPath: string;
  askDownloadLocation: boolean;
  enableNotifications: boolean;
  enableGeolocation: boolean;
  enableCamera: boolean;
  enableMicrophone: boolean;
  sidebarPosition: 'left' | 'right';
  tabPosition: 'top' | 'left' | 'right';
  showTabPreviews: boolean;
  enableReaderMode: boolean;
  readerFontFamily: string;
  readerFontSize: number;
  readerLineHeight: number;
  readerBackground: 'white' | 'sepia' | 'dark';
  enableDevTools: boolean;
  enableExtensions: boolean;
  contentBlockingLevel: 'standard' | 'strict' | 'custom';
  cookieLifetime: 'session' | '1day' | '1week' | '1month' | 'forever';
  enableSafeSearch: boolean;
  enablePopupBlocker: boolean;
  enableAutoplay: 'allow' | 'block' | 'ask';
  enableWebRTC: boolean;
  userAgent: 'default' | 'chrome' | 'firefox' | 'safari' | 'mobile' | 'custom';
  customUserAgent?: string;
  proxySettings?: {
    type: 'none' | 'http' | 'socks5' | 'tor';
    host?: string;
    port?: number;
    username?: string;
    password?: string;
  };
}

export interface BrowserState {
  tabs: Tab[];
  activeTabId: string | null;
  sidebarPanel: SidebarPanel;
  sidebarOpen: boolean;
  settings: Settings;
  isFullscreen: boolean;
  showDevTools: boolean;
  devToolsTab: 'elements' | 'console' | 'network' | 'performance' | 'storage' | 'security';
  showFindBar: boolean;
  findQuery: string;
  findResults: number;
  findCurrentIndex: number;
  showDownloadsPanel: boolean;
  showCommandPalette: boolean;
  commandQuery: string;
  toasts: Toast[];
  gpsLocation?: GeolocationCoordinates;
  networkStatus: 'online' | 'offline' | 'slow';
  isZenMode: boolean;
  pinnedSites: PinnedSite[];
  recentlyClosed: Array<{ url: string; title: string; closedAt: Date }>;
}

export interface Toast {
  id: string;
  type: 'info' | 'success' | 'warning' | 'error';
  message: string;
  duration?: number;
}

export interface PinnedSite {
  url: string;
  title: string;
  favicon?: string;
  color?: string;
}

const DEFAULT_SETTINGS: Settings = {
  theme: 'system',
  language: 'en',
  searchEngine: 'google',
  homepage: 'about:newtab',
  newTabPage: 'default',
  showBookmarksBar: true,
  showStatusBar: true,
  enableAnimations: true,
  fontSize: 'medium',
  defaultZoom: 100,
  enableSpellCheck: true,
  enableAutoFill: true,
  enablePasswordManager: true,
  enableSyncHistory: false,
  enableSyncBookmarks: false,
  enableSyncSettings: false,
  privacyMode: 'normal',
  blockTrackers: true,
  blockAds: false,
  blockCookies: 'none',
  enableDoH: true,
  dohProvider: 'https://cloudflare-dns.com/dns-query',
  enableHTTPSOnly: false,
  enableAntiFingerprint: false,
  enableGPC: true,
  enableDNT: true,
  sendCrashReports: false,
  sendUsageStats: false,
  enableHardwareAcceleration: true,
  enablePrefetch: true,
  enablePrerender: false,
  downloadPath: '~/Downloads',
  askDownloadLocation: false,
  enableNotifications: true,
  enableGeolocation: true,
  enableCamera: false,
  enableMicrophone: false,
  sidebarPosition: 'left',
  tabPosition: 'top',
  showTabPreviews: true,
  enableReaderMode: true,
  readerFontFamily: 'Georgia, serif',
  readerFontSize: 18,
  readerLineHeight: 1.8,
  readerBackground: 'white',
  enableDevTools: true,
  enableExtensions: true,
  contentBlockingLevel: 'standard',
  cookieLifetime: 'forever',
  enableSafeSearch: false,
  enablePopupBlocker: true,
  enableAutoplay: 'ask',
  enableWebRTC: true,
  userAgent: 'default',
  proxySettings: { type: 'none' },
};

const SEARCH_ENGINES: Record<SearchEngine, SearchEngineConfig> = {
  google: { id: 'google', name: 'Google', url: 'https://www.google.com/search?q=', suggestUrl: 'https://suggestqueries.google.com/complete/search?client=firefox&q=', icon: '🔍' },
  bing: { id: 'bing', name: 'Bing', url: 'https://www.bing.com/search?q=', icon: '🔍' },
  duckduckgo: { id: 'duckduckgo', name: 'DuckDuckGo', url: 'https://duckduckgo.com/?q=', suggestUrl: 'https://duckduckgo.com/ac/?q=', icon: '🦆' },
  brave: { id: 'brave', name: 'Brave Search', url: 'https://search.brave.com/search?q=', icon: '🦁' },
  ecosia: { id: 'ecosia', name: 'Ecosia', url: 'https://www.ecosia.org/search?q=', icon: '🌱' },
  startpage: { id: 'startpage', name: 'Startpage', url: 'https://www.startpage.com/search?q=', icon: '🔒' },
  custom: { id: 'custom', name: 'Custom', url: '', icon: '⚙️' },
};

function createTab(url: string, isIncognito = false): Tab {
  return {
    id: `tab-${Date.now()}-${Math.random().toString(36).slice(2, 7)}`,
    url,
    displayUrl: url,
    title: url === 'about:newtab' ? 'New Tab' : url,
    status: 'idle',
    securityLevel: url.startsWith('https://') ? 'secure' : url.startsWith('http://') ? 'warning' : 'local',
    isLoading: false,
    canGoBack: false,
    canGoForward: false,
    isIncognito,
    isMuted: false,
    isPinned: false,
    isBookmarked: false,
    zoom: 100,
    history: [url],
    historyIndex: 0,
    createdAt: new Date(),
    lastActiveAt: new Date(),
    readingMode: false,
  };
}

export const useBrowserStore = create<BrowserState & BrowserActions>()(
  persist(
    immer((set, get) => ({
      tabs: [createTab('about:newtab')],
      activeTabId: null,
      sidebarPanel: null,
      sidebarOpen: false,
      settings: DEFAULT_SETTINGS,
      isFullscreen: false,
      showDevTools: false,
      devToolsTab: 'elements',
      showFindBar: false,
      findQuery: '',
      findResults: 0,
      findCurrentIndex: 0,
      showDownloadsPanel: false,
      showCommandPalette: false,
      commandQuery: '',
      toasts: [],
      networkStatus: 'online',
      isZenMode: false,
      pinnedSites: [
        { url: 'https://google.com', title: 'Google', favicon: 'https://www.google.com/favicon.ico' },
        { url: 'https://github.com', title: 'GitHub', favicon: 'https://github.com/favicon.ico' },
        { url: 'https://youtube.com', title: 'YouTube', favicon: 'https://youtube.com/favicon.ico' },
        { url: 'https://twitter.com', title: 'Twitter', favicon: 'https://twitter.com/favicon.ico' },
        { url: 'https://reddit.com', title: 'Reddit', favicon: 'https://reddit.com/favicon.ico' },
        { url: 'https://wikipedia.org', title: 'Wikipedia', favicon: 'https://wikipedia.org/favicon.ico' },
        { url: 'https://amazon.com', title: 'Amazon', favicon: 'https://amazon.com/favicon.ico' },
        { url: 'https://netflix.com', title: 'Netflix', favicon: 'https://netflix.com/favicon.ico' },
      ],
      recentlyClosed: [],

      // Actions
      openTab: (url = 'about:newtab', isIncognito) => set(state => {
        const incognito = isIncognito ?? state.tabs.find(t => t.id === state.activeTabId)?.isIncognito ?? false;
        const tab = createTab(url, incognito);
        state.tabs.push(tab);
        state.activeTabId = tab.id;
      }),

      closeTab: (id) => set(state => {
        const idx = state.tabs.findIndex(t => t.id === id);
        if (idx === -1) return;
        const tab = state.tabs[idx];
        state.recentlyClosed.unshift({ url: tab.url, title: tab.title, closedAt: new Date() });
        if (state.recentlyClosed.length > 20) state.recentlyClosed.pop();
        state.tabs.splice(idx, 1);
        if (state.tabs.length === 0) {
          const newTab = createTab('about:newtab');
          state.tabs.push(newTab);
          state.activeTabId = newTab.id;
        } else if (state.activeTabId === id) {
          state.activeTabId = state.tabs[Math.min(idx, state.tabs.length - 1)].id;
        }
      }),

      setActiveTab: (id) => set(state => {
        state.activeTabId = id;
        const tab = state.tabs.find(t => t.id === id);
        if (tab) tab.lastActiveAt = new Date();
      }),

      updateTab: (id, changes) => set(state => {
        const tab = state.tabs.find(t => t.id === id);
        if (tab) Object.assign(tab, changes);
      }),

      navigateTo: (url, tabId) => set(state => {
        const id = tabId ?? state.activeTabId;
        const tab = state.tabs.find(t => t.id === id);
        if (!tab) return;
        // Push to tab history
        if (tab.historyIndex < tab.history.length - 1) {
          tab.history = tab.history.slice(0, tab.historyIndex + 1);
        }
        tab.history.push(url);
        tab.historyIndex = tab.history.length - 1;
        tab.url = url;
        tab.displayUrl = url;
        tab.status = 'loading';
        tab.isLoading = true;
        tab.canGoBack = tab.historyIndex > 0;
        tab.canGoForward = false;
        tab.securityLevel = url.startsWith('https://') ? 'secure' : url.startsWith('http://') ? 'warning' : 'local';
      }),

      goBack: (tabId) => set(state => {
        const id = tabId ?? state.activeTabId;
        const tab = state.tabs.find(t => t.id === id);
        if (!tab || tab.historyIndex <= 0) return;
        tab.historyIndex--;
        tab.url = tab.history[tab.historyIndex];
        tab.displayUrl = tab.url;
        tab.canGoBack = tab.historyIndex > 0;
        tab.canGoForward = true;
        tab.status = 'loading';
        tab.isLoading = true;
      }),

      goForward: (tabId) => set(state => {
        const id = tabId ?? state.activeTabId;
        const tab = state.tabs.find(t => t.id === id);
        if (!tab || tab.historyIndex >= tab.history.length - 1) return;
        tab.historyIndex++;
        tab.url = tab.history[tab.historyIndex];
        tab.displayUrl = tab.url;
        tab.canGoBack = true;
        tab.canGoForward = tab.historyIndex < tab.history.length - 1;
        tab.status = 'loading';
        tab.isLoading = true;
      }),

      reload: (tabId) => set(state => {
        const id = tabId ?? state.activeTabId;
        const tab = state.tabs.find(t => t.id === id);
        if (tab) { tab.status = 'loading'; tab.isLoading = true; }
      }),

      pinTab: (id) => set(state => {
        const tab = state.tabs.find(t => t.id === id);
        if (tab) tab.isPinned = !tab.isPinned;
      }),

      muteTab: (id) => set(state => {
        const tab = state.tabs.find(t => t.id === id);
        if (tab) tab.isMuted = !tab.isMuted;
      }),

      duplicateTab: (id) => set(state => {
        const tab = state.tabs.find(t => t.id === id);
        if (!tab) return;
        const newTab = { ...createTab(tab.url, tab.isIncognito), title: tab.title, favicon: tab.favicon };
        const idx = state.tabs.findIndex(t => t.id === id);
        state.tabs.splice(idx + 1, 0, newTab);
        state.activeTabId = newTab.id;
      }),

      moveTab: (fromId, toIndex) => set(state => {
        const fromIndex = state.tabs.findIndex(t => t.id === fromId);
        if (fromIndex === -1) return;
        const [tab] = state.tabs.splice(fromIndex, 1);
        state.tabs.splice(toIndex, 0, tab);
      }),

      openIncognito: (url) => set(state => {
        const tab = createTab(url ?? 'about:newtab', true);
        state.tabs.push(tab);
        state.activeTabId = tab.id;
      }),

      closeAllTabs: () => set(state => {
        state.recentlyClosed.unshift(...state.tabs.map(t => ({ url: t.url, title: t.title, closedAt: new Date() })));
        const newTab = createTab('about:newtab');
        state.tabs = [newTab];
        state.activeTabId = newTab.id;
      }),

      closeOtherTabs: (id) => set(state => {
        const tab = state.tabs.find(t => t.id === id);
        if (!tab) return;
        state.recentlyClosed.unshift(...state.tabs.filter(t => t.id !== id).map(t => ({ url: t.url, title: t.title, closedAt: new Date() })));
        state.tabs = [tab];
        state.activeTabId = id;
      }),

      reopenLastClosed: () => set(state => {
        const last = state.recentlyClosed.shift();
        if (!last) return;
        const tab = createTab(last.url);
        tab.title = last.title;
        state.tabs.push(tab);
        state.activeTabId = tab.id;
      }),

      setSidebarPanel: (panel) => set(state => {
        if (state.sidebarPanel === panel) {
          state.sidebarOpen = !state.sidebarOpen;
        } else {
          state.sidebarPanel = panel;
          state.sidebarOpen = true;
        }
      }),

      closeSidebar: () => set(state => { state.sidebarOpen = false; }),

      updateSettings: (changes) => set(state => { Object.assign(state.settings, changes); }),

      setTheme: (theme) => set(state => { state.settings.theme = theme; }),

      toggleFullscreen: () => set(state => { state.isFullscreen = !state.isFullscreen; }),

      toggleDevTools: () => set(state => { state.showDevTools = !state.showDevTools; }),

      setDevToolsTab: (tab) => set(state => { state.devToolsTab = tab; }),

      toggleFindBar: () => set(state => { state.showFindBar = !state.showFindBar; if (!state.showFindBar) state.findQuery = ''; }),

      setFindQuery: (q) => set(state => { state.findQuery = q; }),

      toggleDownloadsPanel: () => set(state => { state.showDownloadsPanel = !state.showDownloadsPanel; }),

      toggleCommandPalette: () => set(state => { state.showCommandPalette = !state.showCommandPalette; if (!state.showCommandPalette) state.commandQuery = ''; }),

      setCommandQuery: (q) => set(state => { state.commandQuery = q; }),

      addToast: (toast) => set(state => {
        state.toasts.push({ id: `toast-${Date.now()}`, duration: 3000, ...toast });
      }),

      removeToast: (id) => set(state => {
        state.toasts = state.toasts.filter(t => t.id !== id);
      }),

      setGpsLocation: (coords) => set(state => { state.gpsLocation = coords; }),

      setNetworkStatus: (status) => set(state => { state.networkStatus = status; }),

      toggleZenMode: () => set(state => { state.isZenMode = !state.isZenMode; }),

      addPinnedSite: (site) => set(state => { state.pinnedSites.push(site); }),

      removePinnedSite: (url) => set(state => { state.pinnedSites = state.pinnedSites.filter(s => s.url !== url); }),

      getActiveTab: () => {
        const state = get();
        return state.tabs.find(t => t.id === state.activeTabId) ?? state.tabs[0] ?? null;
      },

      getSearchEngines: () => SEARCH_ENGINES,

      getSearchUrl: (query) => {
        const state = get();
        const engine = SEARCH_ENGINES[state.settings.searchEngine];
        const url = state.settings.searchEngine === 'custom' ? (state.settings.customSearchUrl ?? engine.url) : engine.url;
        return url + encodeURIComponent(query);
      },
    })),
    {
      name: 'ebrowser-settings',
      storage: createJSONStorage(() => localStorage),
      partialize: (state) => ({
        settings: state.settings,
        pinnedSites: state.pinnedSites,
        recentlyClosed: state.recentlyClosed.slice(0, 10),
      }),
    }
  )
);

export interface BrowserActions {
  openTab: (url?: string, isIncognito?: boolean) => void;
  closeTab: (id: string) => void;
  setActiveTab: (id: string) => void;
  updateTab: (id: string, changes: Partial<Tab>) => void;
  navigateTo: (url: string, tabId?: string) => void;
  goBack: (tabId?: string) => void;
  goForward: (tabId?: string) => void;
  reload: (tabId?: string) => void;
  pinTab: (id: string) => void;
  muteTab: (id: string) => void;
  duplicateTab: (id: string) => void;
  moveTab: (fromId: string, toIndex: number) => void;
  openIncognito: (url?: string) => void;
  closeAllTabs: () => void;
  closeOtherTabs: (id: string) => void;
  reopenLastClosed: () => void;
  setSidebarPanel: (panel: SidebarPanel) => void;
  closeSidebar: () => void;
  updateSettings: (changes: Partial<Settings>) => void;
  setTheme: (theme: ThemeMode) => void;
  toggleFullscreen: () => void;
  toggleDevTools: () => void;
  setDevToolsTab: (tab: BrowserState['devToolsTab']) => void;
  toggleFindBar: () => void;
  setFindQuery: (q: string) => void;
  toggleDownloadsPanel: () => void;
  toggleCommandPalette: () => void;
  setCommandQuery: (q: string) => void;
  addToast: (toast: Omit<Toast, 'id'>) => void;
  removeToast: (id: string) => void;
  setGpsLocation: (coords: GeolocationCoordinates) => void;
  setNetworkStatus: (status: BrowserState['networkStatus']) => void;
  toggleZenMode: () => void;
  addPinnedSite: (site: PinnedSite) => void;
  removePinnedSite: (url: string) => void;
  getActiveTab: () => Tab | null;
  getSearchEngines: () => Record<SearchEngine, SearchEngineConfig>;
  getSearchUrl: (query: string) => string;
}
