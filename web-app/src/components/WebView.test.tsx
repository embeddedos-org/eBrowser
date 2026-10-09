import { describe, expect, it, vi, beforeEach } from 'vitest';
import type { ReactNode } from 'react';
import { render } from '@testing-library/react';

vi.mock('@/utils/database', () => ({
  bookmarkDB: {
    getAll: vi.fn(),
    search: vi.fn(),
    update: vi.fn(),
    delete: vi.fn(),
  },
  historyDB: {
    add: vi.fn(),
    getAll: vi.fn(),
    search: vi.fn(),
    delete: vi.fn(),
    clear: vi.fn(),
  },
}));

vi.mock('react-i18next', () => ({
  useTranslation: () => ({ t: (k: string) => k }),
}));

vi.mock('@/store/browserStore', () => ({
  useBrowserStore: () => ({
    updateTab: vi.fn(),
    addToast: vi.fn(),
    getActiveTab: () => null,
  }),
}));

vi.mock('framer-motion', () => ({
  motion: {
    div: ({ children, ...props }: { children?: ReactNode; [key: string]: unknown }) => (
      <div {...props}>{children}</div>
    ),
  },
}));

import WebView from './WebView';
import type { Tab } from '@/store/browserStore';

function makeTab(url: string): Tab {
  return {
    id: 'tab-1',
    url,
    displayUrl: url,
    title: 'Example',
    status: 'complete',
    securityLevel: 'secure',
    isLoading: false,
    canGoBack: false,
    canGoForward: false,
    isIncognito: false,
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

beforeEach(() => {
  vi.clearAllMocks();
});

describe('WebView referrer policy (privacy invariant)', () => {
  it('renders the page iframe with origin-only referrer policy', () => {
    const { container } = render(<WebView tab={makeTab('https://example.com/page')} />);
    const iframe = container.querySelector('iframe');
    expect(iframe).not.toBeNull();
    // docs/privacy-invariants.md, "Mode defaults": the default referrer
    // policy is origin-only — a navigation may under-disclose, never leak
    // a full URL to another origin.
    expect(iframe!.getAttribute('referrerpolicy')).toBe('origin');
  });
});
