import React, { useRef, useEffect, useCallback, useState } from 'react';
import { useTranslation } from 'react-i18next';
import { RefreshCw, Home, AlertTriangle, WifiOff, ShieldX, Globe } from 'lucide-react';
import { useBrowserStore, type Tab } from '@/store/browserStore';
import { historyDB } from '@/utils/database';
import { isInternalPage, getSecurityLevel, getDomain } from '@/utils/url';
import NewTabPage from './pages/NewTabPage';
import HistoryPage from './pages/HistoryPage';
import BookmarksPage from './pages/BookmarksPage';
import DownloadsPage from './pages/DownloadsPage';
import SettingsPage from './pages/SettingsPage';
import ExtensionsPage from './pages/ExtensionsPage';
import AboutPage from './pages/AboutPage';
import PrivacyPage from './pages/PrivacyPage';
import ReadingListPage from './pages/ReadingListPage';
import NotesPage from './pages/NotesPage';
import PasswordsPage from './pages/PasswordsPage';
import GPSPage from './pages/GPSPage';

interface Props {
  tab: Tab;
}

export default function WebView({ tab }: Props) {
  const { t } = useTranslation();
  const store = useBrowserStore();
  const iframeRef = useRef<HTMLIFrameElement>(null);
  const [error, setError] = useState<string | null>(null);
  const [iframeKey, setIframeKey] = useState(0);

  // Determine if this is an internal page
  const internalPage = getInternalPage(tab.url);

  const handleIframeLoad = useCallback(() => {
    if (!iframeRef.current) return;
    try {
      const iframe = iframeRef.current;
      const title = iframe.contentDocument?.title || getDomain(tab.url);
      const favicon = `https://www.google.com/s2/favicons?domain=${getDomain(tab.url)}&sz=32`;

      store.updateTab(tab.id, {
        title: title || getDomain(tab.url),
        favicon,
        status: 'complete',
        isLoading: false,
        securityLevel: getSecurityLevel(tab.url),
      });

      // Add to history (skip internal pages)
      if (!isInternalPage(tab.url) && tab.url !== 'about:blank') {
        historyDB.add(tab.url, title || getDomain(tab.url), favicon);
      }
      setError(null);
    } catch {
      // Cross-origin — still mark as loaded
      store.updateTab(tab.id, {
        status: 'complete',
        isLoading: false,
        favicon: `https://www.google.com/s2/favicons?domain=${getDomain(tab.url)}&sz=32`,
      });
      setError(null);
    }
  }, [tab.id, tab.url, store]);

  const handleIframeError = useCallback(() => {
    store.updateTab(tab.id, { status: 'error', isLoading: false });
    setError('connection_failed');
  }, [tab.id, store]);

  // Handle reload
  useEffect(() => {
    if (tab.status === 'loading' && !internalPage && iframeRef.current) {
      setIframeKey(k => k + 1);
      setError(null);
    }
  }, [tab.status, internalPage]);

  // Apply zoom
  useEffect(() => {
    if (iframeRef.current) {
      iframeRef.current.style.transform = `scale(${tab.zoom / 100})`;
      iframeRef.current.style.transformOrigin = 'top left';
      iframeRef.current.style.width = `${100 * (100 / tab.zoom)}%`;
      iframeRef.current.style.height = `${100 * (100 / tab.zoom)}%`;
    }
  }, [tab.zoom]);

  // Render internal pages
  if (internalPage) {
    return (
      <div className="w-full h-full overflow-auto bg-white dark:bg-gray-900">
        {internalPage}
      </div>
    );
  }

  // Error page
  if (error) {
    return <ErrorPage error={error} url={tab.url} onRetry={() => { setError(null); setIframeKey(k => k + 1); store.reload(tab.id); }} />;
  }

  // Blank page
  if (tab.url === 'about:blank') {
    return <div className="w-full h-full bg-white dark:bg-gray-900" />;
  }

  // Reading mode overlay
  if (tab.readingMode) {
    return (
      <div className="w-full h-full overflow-auto bg-white dark:bg-gray-900">
        <div className="reader-mode-content">
          <h1 className="text-2xl font-bold mb-4">{tab.title}</h1>
          <p className="text-gray-500 text-sm mb-6">{tab.url}</p>
          <div className="prose dark:prose-invert max-w-none">
            <p>Reader mode is active. The page content would be extracted and displayed here in a clean, distraction-free format.</p>
            <p>In a production environment, this would use the Mozilla Readability library to extract the main article content from the page.</p>
          </div>
        </div>
      </div>
    );
  }

  return (
    <div className="w-full h-full relative overflow-hidden bg-white dark:bg-gray-900">
      {/* Loading overlay */}
      {tab.isLoading && (
        <div className="absolute top-0 left-0 right-0 h-0.5 z-10">
          <div className="h-full bg-primary-500 animate-pulse" style={{ width: '70%' }} />
        </div>
      )}

      <iframe
        ref={iframeRef}
        key={iframeKey}
        src={tab.url}
        className="w-full h-full border-0"
        onLoad={handleIframeLoad}
        onError={handleIframeError}
        title={tab.title || tab.url}
        sandbox="allow-scripts allow-same-origin allow-forms allow-popups allow-downloads allow-modals allow-orientation-lock allow-pointer-lock allow-presentation allow-top-navigation-by-user-activation"
        allow="geolocation; camera; microphone; fullscreen; payment; autoplay; clipboard-read; clipboard-write"
        referrerPolicy="strict-origin-when-cross-origin"
      />

      {/* Incognito overlay indicator */}
      {tab.isIncognito && (
        <div className="absolute top-2 right-2 flex items-center gap-1 bg-gray-800/80 text-white text-xs px-2 py-1 rounded-full pointer-events-none">
          <Globe size={10} />
          Incognito
        </div>
      )}
    </div>
  );
}

function getInternalPage(url: string): React.ReactNode | null {
  if (url === 'about:newtab' || url === 'about:home' || url === '') return <NewTabPage />;
  if (url === 'about:blank') return null;
  if (url === 'ebrowser://history') return <HistoryPage />;
  if (url === 'ebrowser://bookmarks') return <BookmarksPage />;
  if (url === 'ebrowser://downloads') return <DownloadsPage />;
  if (url === 'ebrowser://settings') return <SettingsPage />;
  if (url === 'ebrowser://extensions') return <ExtensionsPage />;
  if (url === 'ebrowser://about' || url === 'ebrowser://version') return <AboutPage />;
  if (url === 'ebrowser://privacy' || url === 'ebrowser://security') return <PrivacyPage />;
  if (url === 'ebrowser://reading-list') return <ReadingListPage />;
  if (url === 'ebrowser://notes') return <NotesPage />;
  if (url === 'ebrowser://passwords') return <PasswordsPage />;
  if (url === 'ebrowser://gps') return <GPSPage />;
  return null;
}

interface ErrorPageProps {
  error: string;
  url: string;
  onRetry: () => void;
}

function ErrorPage({ error, url, onRetry }: ErrorPageProps) {
  const { t } = useTranslation();
  const store = useBrowserStore();

  const errorConfig: Record<string, { icon: React.ReactNode; title: string; description: string }> = {
    connection_failed: {
      icon: <WifiOff size={48} className="text-gray-400" />,
      title: t('errors.connectionFailed'),
      description: 'Unable to connect to the server. The site may be down or you may have a network issue.',
    },
    dns_error: {
      icon: <Globe size={48} className="text-gray-400" />,
      title: t('errors.dnsError'),
      description: 'The DNS lookup for this domain failed.',
    },
    ssl_error: {
      icon: <ShieldX size={48} className="text-red-400" />,
      title: t('errors.sslError'),
      description: 'The SSL certificate for this site is invalid or expired.',
    },
    blocked: {
      icon: <AlertTriangle size={48} className="text-yellow-400" />,
      title: t('errors.blocked'),
      description: 'This page has been blocked by your privacy settings.',
    },
  };

  const config = errorConfig[error] ?? {
    icon: <AlertTriangle size={48} className="text-gray-400" />,
    title: t('errors.unknown'),
    description: 'An unexpected error occurred while loading this page.',
  };

  return (
    <div className="w-full h-full flex items-center justify-center bg-white dark:bg-gray-900 p-8">
      <div className="max-w-md w-full text-center space-y-6">
        <div className="flex justify-center">{config.icon}</div>
        <div>
          <h1 className="text-xl font-semibold text-gray-900 dark:text-gray-100 mb-2">{config.title}</h1>
          <p className="text-gray-500 dark:text-gray-400 text-sm">{config.description}</p>
          <p className="text-gray-400 dark:text-gray-500 text-xs mt-2 font-mono truncate">{url}</p>
        </div>
        <div className="flex gap-3 justify-center">
          <button onClick={onRetry} className="btn-primary gap-2">
            <RefreshCw size={14} />
            {t('errors.tryAgain')}
          </button>
          <button onClick={() => store.navigateTo('about:newtab')} className="btn-secondary gap-2">
            <Home size={14} />
            {t('errors.goHome')}
          </button>
        </div>
      </div>
    </div>
  );
}
