import React, { useEffect, useCallback } from 'react';
import { useTranslation } from 'react-i18next';
import { AnimatePresence } from 'framer-motion';
import { useBrowserStore } from '@/store/browserStore';
import BrowserShell from '@/components/BrowserShell';
import ToastContainer from '@/components/ToastContainer';
import CommandPalette from '@/components/CommandPalette';
import { useKeyboardShortcuts } from '@/hooks/useKeyboardShortcuts';
import { useTheme } from '@/hooks/useTheme';
import { useNetworkStatus } from '@/hooks/useNetworkStatus';
import { useGPS } from '@/hooks/useGPS';

export default function App() {
  const { i18n } = useTranslation();
  const { settings, showCommandPalette } = useBrowserStore();

  // Apply theme
  useTheme();

  // Network status monitoring
  useNetworkStatus();

  // GPS location tracking
  useGPS();

  // Keyboard shortcuts
  useKeyboardShortcuts();

  // Apply language direction (RTL for Arabic, etc.)
  useEffect(() => {
    const lang = settings.language;
    const rtlLanguages = ['ar', 'he', 'fa', 'ur'];
    document.documentElement.dir = rtlLanguages.includes(lang) ? 'rtl' : 'ltr';
    document.documentElement.lang = lang;
    if (lang !== i18n.language) {
      i18n.changeLanguage(lang);
    }
  }, [settings.language, i18n]);

  // Apply font size
  useEffect(() => {
    const fontSizeMap: Record<string, string> = {
      small: '13px',
      medium: '14px',
      large: '16px',
      xlarge: '18px',
    };
    document.documentElement.style.fontSize = fontSizeMap[settings.fontSize] || '14px';
  }, [settings.fontSize]);

  // Prevent default context menu (browser handles it)
  const handleContextMenu = useCallback((e: React.MouseEvent) => {
    const target = e.target as HTMLElement;
    if (!target.closest('[data-allow-context-menu]')) {
      e.preventDefault();
    }
  }, []);

  return (
    <div
      className="h-full flex flex-col overflow-hidden select-none"
      onContextMenu={handleContextMenu}
    >
      <BrowserShell />

      {/* Toast notifications */}
      <ToastContainer />

      {/* Command Palette */}
      <AnimatePresence>
        {showCommandPalette && <CommandPalette />}
      </AnimatePresence>
    </div>
  );
}
