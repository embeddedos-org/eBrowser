/**
 * eBrowser URL Utilities
 * Handles URL parsing, validation, normalization, and security checks.
 */

export interface ParsedURL {
  href: string;
  protocol: string;
  hostname: string;
  port: string;
  pathname: string;
  search: string;
  hash: string;
  origin: string;
  isInternal: boolean;
  isSecure: boolean;
  displayUrl: string;
  faviconUrl: string;
  tld?: string;
  domain?: string;
}

const INTERNAL_SCHEMES = new Set(['about:', 'chrome:', 'ebrowser:', 'data:', 'blob:', 'javascript:']);
const SEARCH_ENGINES_PATTERNS = [
  /^(www\.)?google\./,
  /^(www\.)?bing\./,
  /^(www\.)?duckduckgo\./,
  /^(www\.)?yahoo\./,
  /^search\.brave\./,
  /^(www\.)?ecosia\./,
];

export function parseURL(input: string): ParsedURL | null {
  try {
    const url = new URL(input);
    const isInternal = INTERNAL_SCHEMES.has(url.protocol) || input.startsWith('about:');
    const isSecure = url.protocol === 'https:' || isInternal;
    const hostname = url.hostname;
    const parts = hostname.split('.');
    const tld = parts.length >= 2 ? parts.slice(-2).join('.') : hostname;
    const domain = parts.length >= 2 ? parts.slice(-2).join('.') : hostname;

    return {
      href: url.href,
      protocol: url.protocol,
      hostname,
      port: url.port,
      pathname: url.pathname,
      search: url.search,
      hash: url.hash,
      origin: url.origin,
      isInternal,
      isSecure,
      displayUrl: formatDisplayUrl(url),
      faviconUrl: isInternal ? '' : `https://www.google.com/s2/favicons?domain=${hostname}&sz=32`,
      tld,
      domain,
    };
  } catch {
    return null;
  }
}

export function formatDisplayUrl(url: URL): string {
  let display = url.hostname;
  if (url.port) display += `:${url.port}`;
  if (url.pathname !== '/') display += url.pathname;
  if (url.search) display += url.search;
  if (url.hash) display += url.hash;
  return display;
}

export function normalizeInput(input: string): string {
  const trimmed = input.trim();
  if (!trimmed) return 'about:newtab';

  // Internal pages
  if (trimmed.startsWith('about:') || trimmed.startsWith('ebrowser:') || trimmed.startsWith('chrome:')) {
    return trimmed;
  }

  // Data URLs
  if (trimmed.startsWith('data:') || trimmed.startsWith('blob:')) {
    return trimmed;
  }

  // Already a valid URL
  try {
    const url = new URL(trimmed);
    return url.href;
  } catch {
    // Not a valid URL
  }

  // Has protocol-like prefix
  if (/^[a-zA-Z][a-zA-Z0-9+\-.]*:\/\//.test(trimmed)) {
    try {
      return new URL(trimmed).href;
    } catch {
      return `https://www.google.com/search?q=${encodeURIComponent(trimmed)}`;
    }
  }

  // Looks like a domain (contains a dot, no spaces)
  if (!trimmed.includes(' ') && trimmed.includes('.') && !trimmed.startsWith('.')) {
    const withProtocol = `https://${trimmed}`;
    try {
      const url = new URL(withProtocol);
      if (url.hostname.includes('.')) return url.href;
    } catch {
      // fall through to search
    }
  }

  // Treat as search query
  return `https://www.google.com/search?q=${encodeURIComponent(trimmed)}`;
}

export function isSearchQuery(input: string): boolean {
  const trimmed = input.trim();
  if (trimmed.includes(' ')) return true;
  if (trimmed.startsWith('about:') || trimmed.startsWith('ebrowser:')) return false;
  try {
    const url = new URL(trimmed.includes('://') ? trimmed : `https://${trimmed}`);
    return !url.hostname.includes('.');
  } catch {
    return true;
  }
}

export function getSecurityLevel(url: string): 'secure' | 'warning' | 'danger' | 'local' | 'extension' {
  if (!url) return 'local';
  if (url.startsWith('about:') || url.startsWith('ebrowser:') || url.startsWith('chrome:')) return 'local';
  if (url.startsWith('chrome-extension:') || url.startsWith('moz-extension:')) return 'extension';
  try {
    const parsed = new URL(url);
    if (parsed.protocol === 'https:') return 'secure';
    if (parsed.protocol === 'http:') return 'warning';
    return 'local';
  } catch {
    return 'local';
  }
}

export function getDomain(url: string): string {
  try {
    return new URL(url).hostname;
  } catch {
    return url;
  }
}

export function getFaviconUrl(url: string, size = 32): string {
  try {
    const parsed = new URL(url);
    if (parsed.protocol === 'https:' || parsed.protocol === 'http:') {
      return `https://www.google.com/s2/favicons?domain=${parsed.hostname}&sz=${size}`;
    }
  } catch {
    // ignore
  }
  return '';
}

export function cleanTrackingParams(url: string): string {
  const TRACKING_PARAMS = new Set([
    'utm_source', 'utm_medium', 'utm_campaign', 'utm_term', 'utm_content',
    'fbclid', 'gclid', 'msclkid', 'mc_eid', 'ref', 'referrer',
    '_ga', '_gl', 'yclid', 'zanpid', 'dclid', 'igshid',
    'twclid', 'li_fat_id', 'ttclid', 'epik', 'pp',
  ]);

  try {
    const parsed = new URL(url);
    const toDelete: string[] = [];
    parsed.searchParams.forEach((_, key) => {
      if (TRACKING_PARAMS.has(key.toLowerCase())) toDelete.push(key);
    });
    toDelete.forEach(k => parsed.searchParams.delete(k));
    return parsed.toString();
  } catch {
    return url;
  }
}

export function isBlockedDomain(hostname: string, blocklist: string[]): boolean {
  return blocklist.some(pattern => {
    if (pattern.startsWith('*.')) {
      const domain = pattern.slice(2);
      return hostname === domain || hostname.endsWith(`.${domain}`);
    }
    return hostname === pattern;
  });
}

export function extractPageTitle(html: string): string {
  const match = html.match(/<title[^>]*>([^<]*)<\/title>/i);
  return match ? match[1].trim() : '';
}

export function isValidURL(url: string): boolean {
  try {
    new URL(url);
    return true;
  } catch {
    return false;
  }
}

export function resolveRelativeURL(base: string, relative: string): string {
  try {
    return new URL(relative, base).href;
  } catch {
    return relative;
  }
}

export function getReadableFileSize(bytes: number): string {
  if (bytes === 0) return '0 B';
  const k = 1024;
  const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return `${parseFloat((bytes / Math.pow(k, i)).toFixed(1))} ${sizes[i]}`;
}

export function formatURL(url: string): string {
  try {
    const parsed = new URL(url);
    return parsed.hostname + (parsed.pathname !== '/' ? parsed.pathname : '');
  } catch {
    return url;
  }
}

export const INTERNAL_PAGES = {
  NEWTAB: 'about:newtab',
  HOME: 'about:home',
  BLANK: 'about:blank',
  HISTORY: 'ebrowser://history',
  BOOKMARKS: 'ebrowser://bookmarks',
  DOWNLOADS: 'ebrowser://downloads',
  SETTINGS: 'ebrowser://settings',
  EXTENSIONS: 'ebrowser://extensions',
  DEVTOOLS: 'ebrowser://devtools',
  ABOUT: 'ebrowser://about',
  VERSION: 'ebrowser://version',
  FLAGS: 'ebrowser://flags',
  PRIVACY: 'ebrowser://privacy',
  SECURITY: 'ebrowser://security',
  READING_LIST: 'ebrowser://reading-list',
  NOTES: 'ebrowser://notes',
  SYNC: 'ebrowser://sync',
  PASSWORDS: 'ebrowser://passwords',
  COOKIES: 'ebrowser://cookies',
  SITE_DATA: 'ebrowser://site-data',
  PERFORMANCE: 'ebrowser://performance',
  NETWORK: 'ebrowser://network',
  GPS: 'ebrowser://gps',
};

export function isInternalPage(url: string): boolean {
  return url.startsWith('about:') || url.startsWith('ebrowser://') || url.startsWith('chrome://');
}

export function getInternalPageName(url: string): string {
  if (url === INTERNAL_PAGES.NEWTAB || url === INTERNAL_PAGES.HOME) return 'New Tab';
  if (url === INTERNAL_PAGES.BLANK) return 'Blank';
  if (url.startsWith('ebrowser://')) return url.replace('ebrowser://', '').replace(/-/g, ' ').replace(/\b\w/g, c => c.toUpperCase());
  return url;
}
