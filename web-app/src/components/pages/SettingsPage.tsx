import React, { useState } from 'react';
import { useTranslation } from 'react-i18next';
import {
  Settings, Palette, Shield, Search, Download, Globe, Accessibility,
  Cpu, Info, ChevronRight, Moon, Sun, Monitor, Check, Lock,
  Bell, MapPin, Camera, Mic, Database, Trash2, RefreshCw, Zap
} from 'lucide-react';
import { useBrowserStore, type Settings as SettingsType } from '@/store/browserStore';
import { SUPPORTED_LANGUAGES } from '@/i18n/config';
import i18n from '@/i18n/config';

type Section = 'general' | 'appearance' | 'privacy' | 'search' | 'downloads' | 'languages' | 'accessibility' | 'advanced' | 'about';

const SECTIONS: Array<{ id: Section; icon: React.ReactNode; label: string }> = [
  { id: 'general', icon: <Settings size={16} />, label: 'General' },
  { id: 'appearance', icon: <Palette size={16} />, label: 'Appearance' },
  { id: 'privacy', icon: <Shield size={16} />, label: 'Privacy & Security' },
  { id: 'search', icon: <Search size={16} />, label: 'Search' },
  { id: 'downloads', icon: <Download size={16} />, label: 'Downloads' },
  { id: 'languages', icon: <Globe size={16} />, label: 'Languages' },
  { id: 'accessibility', icon: <Accessibility size={16} />, label: 'Accessibility' },
  { id: 'advanced', icon: <Cpu size={16} />, label: 'Advanced' },
  { id: 'about', icon: <Info size={16} />, label: 'About' },
];

export default function SettingsPage() {
  const { t } = useTranslation();
  const { settings, updateSettings, addToast } = useBrowserStore();
  const [activeSection, setActiveSection] = useState<Section>('general');
  const [saved, setSaved] = useState(false);

  const handleChange = <K extends keyof SettingsType>(key: K, value: SettingsType[K]) => {
    updateSettings({ [key]: value });
    if (key === 'language') {
      i18n.changeLanguage(value as string);
    }
    setSaved(true);
    setTimeout(() => setSaved(false), 2000);
  };

  return (
    <div className="flex h-full bg-gray-50 dark:bg-gray-900">
      {/* Sidebar */}
      <div className="w-56 flex-shrink-0 bg-white dark:bg-gray-800 border-r border-gray-200 dark:border-gray-700 overflow-y-auto">
        <div className="p-4 border-b border-gray-200 dark:border-gray-700">
          <h1 className="text-lg font-semibold text-gray-900 dark:text-gray-100">{t('settings.title')}</h1>
        </div>
        <nav className="p-2 space-y-0.5">
          {SECTIONS.map(section => (
            <button
              key={section.id}
              onClick={() => setActiveSection(section.id)}
              className={`w-full flex items-center gap-2.5 px-3 py-2 rounded-lg text-sm transition-colors ${
                activeSection === section.id
                  ? 'bg-primary-50 dark:bg-primary-900/20 text-primary-600 dark:text-primary-400 font-medium'
                  : 'text-gray-600 dark:text-gray-400 hover:bg-gray-100 dark:hover:bg-gray-700'
              }`}
            >
              {section.icon}
              {section.label}
              {activeSection === section.id && <ChevronRight size={14} className="ml-auto" />}
            </button>
          ))}
        </nav>
      </div>

      {/* Content */}
      <div className="flex-1 overflow-y-auto p-8">
        {saved && (
          <div className="fixed top-4 right-4 flex items-center gap-2 bg-green-500 text-white px-4 py-2 rounded-lg shadow-lg z-50 text-sm">
            <Check size={14} />
            {t('settings.saved')}
          </div>
        )}

        {activeSection === 'general' && (
          <SettingsSection title="General">
            <SettingRow label="Homepage" description="The page that opens when you start eBrowser">
              <input
                type="text"
                value={settings.homepage}
                onChange={(e) => handleChange('homepage', e.target.value)}
                className="input w-64"
              />
            </SettingRow>
            <SettingRow label="New Tab Page" description="What to show when you open a new tab">
              <select
                value={settings.newTabPage}
                onChange={(e) => handleChange('newTabPage', e.target.value as SettingsType['newTabPage'])}
                className="input w-48"
              >
                <option value="default">Default (eBrowser)</option>
                <option value="blank">Blank Page</option>
                <option value="custom">Custom URL</option>
              </select>
            </SettingRow>
            <SettingRow label="Show Bookmarks Bar" description="Display bookmarks bar below the toolbar">
              <Toggle checked={settings.showBookmarksBar} onChange={(v) => handleChange('showBookmarksBar', v)} />
            </SettingRow>
            <SettingRow label="Show Status Bar" description="Show URL and status information at the bottom">
              <Toggle checked={settings.showStatusBar} onChange={(v) => handleChange('showStatusBar', v)} />
            </SettingRow>
            <SettingRow label="Default Zoom" description="Default zoom level for all pages">
              <select
                value={settings.defaultZoom}
                onChange={(e) => handleChange('defaultZoom', parseInt(e.target.value))}
                className="input w-32"
              >
                {[25, 33, 50, 67, 75, 80, 90, 100, 110, 125, 150, 175, 200, 250, 300].map(z => (
                  <option key={z} value={z}>{z}%</option>
                ))}
              </select>
            </SettingRow>
            <SettingRow label="Spell Check" description="Check spelling in text fields">
              <Toggle checked={settings.enableSpellCheck} onChange={(v) => handleChange('enableSpellCheck', v)} />
            </SettingRow>
            <SettingRow label="Autofill" description="Automatically fill in forms">
              <Toggle checked={settings.enableAutoFill} onChange={(v) => handleChange('enableAutoFill', v)} />
            </SettingRow>
            <SettingRow label="Password Manager" description="Save and fill passwords">
              <Toggle checked={settings.enablePasswordManager} onChange={(v) => handleChange('enablePasswordManager', v)} />
            </SettingRow>
          </SettingsSection>
        )}

        {activeSection === 'appearance' && (
          <SettingsSection title="Appearance">
            <SettingRow label="Theme" description="Choose your preferred color scheme">
              <div className="flex gap-2">
                {(['light', 'dark', 'system'] as const).map(theme => (
                  <button
                    key={theme}
                    onClick={() => handleChange('theme', theme)}
                    className={`flex items-center gap-2 px-3 py-2 rounded-lg border text-sm transition-colors ${
                      settings.theme === theme
                        ? 'border-primary-500 bg-primary-50 dark:bg-primary-900/20 text-primary-600 dark:text-primary-400'
                        : 'border-gray-200 dark:border-gray-700 text-gray-600 dark:text-gray-400 hover:border-gray-300'
                    }`}
                  >
                    {theme === 'light' ? <Sun size={14} /> : theme === 'dark' ? <Moon size={14} /> : <Monitor size={14} />}
                    {theme.charAt(0).toUpperCase() + theme.slice(1)}
                    {settings.theme === theme && <Check size={12} />}
                  </button>
                ))}
              </div>
            </SettingRow>
            <SettingRow label="Font Size" description="Adjust the default font size">
              <select
                value={settings.fontSize}
                onChange={(e) => handleChange('fontSize', e.target.value as SettingsType['fontSize'])}
                className="input w-40"
              >
                <option value="small">Small</option>
                <option value="medium">Medium</option>
                <option value="large">Large</option>
                <option value="xlarge">Extra Large</option>
              </select>
            </SettingRow>
            <SettingRow label="Animations" description="Enable smooth animations and transitions">
              <Toggle checked={settings.enableAnimations} onChange={(v) => handleChange('enableAnimations', v)} />
            </SettingRow>
            <SettingRow label="Show Tab Previews" description="Show page preview on tab hover">
              <Toggle checked={settings.showTabPreviews} onChange={(v) => handleChange('showTabPreviews', v)} />
            </SettingRow>
            <SettingRow label="Reader Mode Font" description="Font family for Reader Mode">
              <select
                value={settings.readerFontFamily}
                onChange={(e) => handleChange('readerFontFamily', e.target.value)}
                className="input w-48"
              >
                <option value="Georgia, serif">Georgia (Serif)</option>
                <option value="'Times New Roman', serif">Times New Roman</option>
                <option value="Inter, sans-serif">Inter (Sans-serif)</option>
                <option value="'JetBrains Mono', monospace">JetBrains Mono</option>
              </select>
            </SettingRow>
            <SettingRow label="Reader Background" description="Background color in Reader Mode">
              <select
                value={settings.readerBackground}
                onChange={(e) => handleChange('readerBackground', e.target.value as SettingsType['readerBackground'])}
                className="input w-40"
              >
                <option value="white">White</option>
                <option value="sepia">Sepia</option>
                <option value="dark">Dark</option>
              </select>
            </SettingRow>
          </SettingsSection>
        )}

        {activeSection === 'privacy' && (
          <SettingsSection title="Privacy & Security">
            <SettingRow label="Block Trackers" description="Block known tracking scripts and pixels">
              <Toggle checked={settings.blockTrackers} onChange={(v) => handleChange('blockTrackers', v)} />
            </SettingRow>
            <SettingRow label="Block Ads" description="Block advertisements across all websites">
              <Toggle checked={settings.blockAds} onChange={(v) => handleChange('blockAds', v)} />
            </SettingRow>
            <SettingRow label="HTTPS-Only Mode" description="Automatically upgrade connections to HTTPS">
              <Toggle checked={settings.enableHTTPSOnly} onChange={(v) => handleChange('enableHTTPSOnly', v)} />
            </SettingRow>
            <SettingRow label="DNS over HTTPS" description="Encrypt DNS queries for privacy">
              <Toggle checked={settings.enableDoH} onChange={(v) => handleChange('enableDoH', v)} />
            </SettingRow>
            <SettingRow label="Anti-Fingerprinting" description="Prevent websites from tracking your browser fingerprint">
              <Toggle checked={settings.enableAntiFingerprint} onChange={(v) => handleChange('enableAntiFingerprint', v)} />
            </SettingRow>
            <SettingRow label="Global Privacy Control" description="Signal your privacy preferences to websites">
              <Toggle checked={settings.enableGPC} onChange={(v) => handleChange('enableGPC', v)} />
            </SettingRow>
            <SettingRow label="Do Not Track" description="Request websites not to track you">
              <Toggle checked={settings.enableDNT} onChange={(v) => handleChange('enableDNT', v)} />
            </SettingRow>
            <SettingRow label="Block Popups" description="Block popup windows">
              <Toggle checked={settings.enablePopupBlocker} onChange={(v) => handleChange('enablePopupBlocker', v)} />
            </SettingRow>
            <SettingRow label="Cookie Policy" description="How to handle third-party cookies">
              <select
                value={settings.blockCookies}
                onChange={(e) => handleChange('blockCookies', e.target.value as SettingsType['blockCookies'])}
                className="input w-48"
              >
                <option value="none">Allow All Cookies</option>
                <option value="third-party">Block Third-Party</option>
                <option value="all">Block All Cookies</option>
              </select>
            </SettingRow>
            <SettingRow label="Location Access" description="Allow websites to request your location">
              <Toggle checked={settings.enableGeolocation} onChange={(v) => handleChange('enableGeolocation', v)} />
            </SettingRow>
            <SettingRow label="Camera Access" description="Allow websites to access your camera">
              <Toggle checked={settings.enableCamera} onChange={(v) => handleChange('enableCamera', v)} />
            </SettingRow>
            <SettingRow label="Microphone Access" description="Allow websites to access your microphone">
              <Toggle checked={settings.enableMicrophone} onChange={(v) => handleChange('enableMicrophone', v)} />
            </SettingRow>
            <div className="pt-4 border-t border-gray-200 dark:border-gray-700">
              <button
                onClick={() => {
                  if (confirm('Clear all browsing data? This cannot be undone.')) {
                    addToast({ type: 'success', message: 'Browsing data cleared' });
                  }
                }}
                className="btn-danger gap-2"
              >
                <Trash2 size={14} />
                Clear All Browsing Data
              </button>
            </div>
          </SettingsSection>
        )}

        {activeSection === 'search' && (
          <SettingsSection title="Search">
            <SettingRow label="Default Search Engine" description="Search engine used in the address bar">
              <select
                value={settings.searchEngine}
                onChange={(e) => handleChange('searchEngine', e.target.value as SettingsType['searchEngine'])}
                className="input w-48"
              >
                <option value="google">Google</option>
                <option value="bing">Bing</option>
                <option value="duckduckgo">DuckDuckGo</option>
                <option value="brave">Brave Search</option>
                <option value="ecosia">Ecosia</option>
                <option value="startpage">Startpage</option>
                <option value="custom">Custom</option>
              </select>
            </SettingRow>
            {settings.searchEngine === 'custom' && (
              <SettingRow label="Custom Search URL" description="Use %s as placeholder for the query">
                <input
                  type="text"
                  value={settings.customSearchUrl ?? ''}
                  onChange={(e) => handleChange('customSearchUrl', e.target.value)}
                  placeholder="https://example.com/search?q=%s"
                  className="input w-80"
                />
              </SettingRow>
            )}
            <SettingRow label="Safe Search" description="Filter explicit content from search results">
              <Toggle checked={settings.enableSafeSearch} onChange={(v) => handleChange('enableSafeSearch', v)} />
            </SettingRow>
          </SettingsSection>
        )}

        {activeSection === 'downloads' && (
          <SettingsSection title="Downloads">
            <SettingRow label="Download Location" description="Where to save downloaded files">
              <input
                type="text"
                value={settings.downloadPath}
                onChange={(e) => handleChange('downloadPath', e.target.value)}
                className="input w-64"
              />
            </SettingRow>
            <SettingRow label="Ask for Location" description="Ask where to save each file before downloading">
              <Toggle checked={settings.askDownloadLocation} onChange={(v) => handleChange('askDownloadLocation', v)} />
            </SettingRow>
          </SettingsSection>
        )}

        {activeSection === 'languages' && (
          <SettingsSection title="Languages">
            <SettingRow label="Interface Language" description="Language used for the browser interface">
              <select
                value={settings.language}
                onChange={(e) => handleChange('language', e.target.value)}
                className="input w-56"
              >
                {SUPPORTED_LANGUAGES.map(lang => (
                  <option key={lang.code} value={lang.code}>
                    {lang.nativeName} ({lang.name})
                  </option>
                ))}
              </select>
            </SettingRow>
            <div className="mt-4">
              <h3 className="text-sm font-medium text-gray-700 dark:text-gray-300 mb-3">Supported Languages</h3>
              <div className="grid grid-cols-2 sm:grid-cols-3 gap-2">
                {SUPPORTED_LANGUAGES.map(lang => (
                  <div
                    key={lang.code}
                    className={`flex items-center gap-2 p-2 rounded-lg border text-sm ${
                      settings.language === lang.code
                        ? 'border-primary-500 bg-primary-50 dark:bg-primary-900/20'
                        : 'border-gray-200 dark:border-gray-700'
                    }`}
                  >
                    <span className="text-base">{lang.dir === 'rtl' ? '←' : '→'}</span>
                    <div>
                      <div className="font-medium text-gray-900 dark:text-gray-100">{lang.nativeName}</div>
                      <div className="text-xs text-gray-400">{lang.name}</div>
                    </div>
                    {settings.language === lang.code && <Check size={12} className="ml-auto text-primary-500" />}
                  </div>
                ))}
              </div>
            </div>
          </SettingsSection>
        )}

        {activeSection === 'advanced' && (
          <SettingsSection title="Advanced">
            <SettingRow label="Hardware Acceleration" description="Use GPU for rendering (recommended)">
              <Toggle checked={settings.enableHardwareAcceleration} onChange={(v) => handleChange('enableHardwareAcceleration', v)} />
            </SettingRow>
            <SettingRow label="Prefetch" description="Preload pages you're likely to visit next">
              <Toggle checked={settings.enablePrefetch} onChange={(v) => handleChange('enablePrefetch', v)} />
            </SettingRow>
            <SettingRow label="WebRTC" description="Enable real-time communication features">
              <Toggle checked={settings.enableWebRTC} onChange={(v) => handleChange('enableWebRTC', v)} />
            </SettingRow>
            <SettingRow label="Developer Tools" description="Enable the developer tools panel">
              <Toggle checked={settings.enableDevTools} onChange={(v) => handleChange('enableDevTools', v)} />
            </SettingRow>
            <SettingRow label="Extensions" description="Enable browser extensions">
              <Toggle checked={settings.enableExtensions} onChange={(v) => handleChange('enableExtensions', v)} />
            </SettingRow>
            <SettingRow label="User Agent" description="How eBrowser identifies itself to websites">
              <select
                value={settings.userAgent}
                onChange={(e) => handleChange('userAgent', e.target.value as SettingsType['userAgent'])}
                className="input w-48"
              >
                <option value="default">eBrowser Default</option>
                <option value="chrome">Google Chrome</option>
                <option value="firefox">Mozilla Firefox</option>
                <option value="safari">Apple Safari</option>
                <option value="mobile">Mobile Browser</option>
                <option value="custom">Custom</option>
              </select>
            </SettingRow>
            <SettingRow label="Proxy" description="Route traffic through a proxy server">
              <select
                value={settings.proxySettings?.type ?? 'none'}
                onChange={(e) => handleChange('proxySettings', { ...settings.proxySettings, type: e.target.value as 'none' | 'http' | 'socks5' | 'tor' })}
                className="input w-40"
              >
                <option value="none">No Proxy</option>
                <option value="http">HTTP Proxy</option>
                <option value="socks5">SOCKS5 Proxy</option>
                <option value="tor">Tor Network</option>
              </select>
            </SettingRow>
          </SettingsSection>
        )}

        {activeSection === 'about' && (
          <SettingsSection title="About eBrowser">
            <div className="space-y-6">
              <div className="flex items-center gap-4">
                <div className="w-16 h-16 rounded-2xl bg-primary-500 flex items-center justify-center text-white text-2xl font-bold shadow-lg">
                  eB
                </div>
                <div>
                  <h2 className="text-xl font-bold text-gray-900 dark:text-gray-100">eBrowser</h2>
                  <p className="text-gray-500 dark:text-gray-400">Version 2.0.0</p>
                  <p className="text-xs text-gray-400 mt-0.5">The World's Most Powerful Browser</p>
                </div>
              </div>

              <div className="grid grid-cols-2 gap-4">
                {[
                  { label: 'Engine', value: 'eBrowser Engine v2' },
                  { label: 'Platform', value: 'Web / PWA' },
                  { label: 'Security', value: 'TLS 1.3 + DoH' },
                  { label: 'Privacy', value: 'Tracker Blocking' },
                  { label: 'Extensions', value: 'WebExtensions V3' },
                  { label: 'Languages', value: '12 Languages' },
                  { label: 'License', value: 'MIT' },
                  { label: 'Repository', value: 'GitHub' },
                ].map(item => (
                  <div key={item.label} className="card">
                    <div className="text-xs text-gray-400 uppercase tracking-wider">{item.label}</div>
                    <div className="text-sm font-medium text-gray-900 dark:text-gray-100 mt-1">{item.value}</div>
                  </div>
                ))}
              </div>

              <div className="flex gap-3">
                <button className="btn-primary gap-2">
                  <RefreshCw size={14} />
                  Check for Updates
                </button>
                <a
                  href="https://github.com/embeddedos-org/eBrowser"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="btn-secondary gap-2"
                >
                  <Zap size={14} />
                  View on GitHub
                </a>
              </div>

              <div className="text-xs text-gray-400 space-y-1">
                <p>© 2024 EmbeddedOS Organization. All rights reserved.</p>
                <p>Licensed under the MIT License.</p>
                <p>Built with React, TypeScript, and TailwindCSS.</p>
              </div>
            </div>
          </SettingsSection>
        )}
      </div>
    </div>
  );
}

function SettingsSection({ title, children }: { title: string; children: React.ReactNode }) {
  return (
    <div>
      <h2 className="text-xl font-semibold text-gray-900 dark:text-gray-100 mb-6">{title}</h2>
      <div className="space-y-4">{children}</div>
    </div>
  );
}

function SettingRow({ label, description, children }: { label: string; description?: string; children: React.ReactNode }) {
  return (
    <div className="flex items-center justify-between py-3 border-b border-gray-100 dark:border-gray-700/50 last:border-0">
      <div className="flex-1 mr-4">
        <div className="text-sm font-medium text-gray-900 dark:text-gray-100">{label}</div>
        {description && <div className="text-xs text-gray-400 dark:text-gray-500 mt-0.5">{description}</div>}
      </div>
      <div className="flex-shrink-0">{children}</div>
    </div>
  );
}

function Toggle({ checked, onChange }: { checked: boolean; onChange: (v: boolean) => void }) {
  return (
    <button
      role="switch"
      aria-checked={checked}
      onClick={() => onChange(!checked)}
      className={`relative inline-flex h-5 w-9 items-center rounded-full transition-colors duration-200 focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-primary-500 ${
        checked ? 'bg-primary-500' : 'bg-gray-300 dark:bg-gray-600'
      }`}
    >
      <span
        className={`inline-block h-3.5 w-3.5 transform rounded-full bg-white shadow-sm transition-transform duration-200 ${
          checked ? 'translate-x-4.5' : 'translate-x-0.5'
        }`}
        style={{ transform: checked ? 'translateX(18px)' : 'translateX(2px)' }}
      />
    </button>
  );
}
