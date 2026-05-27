import React, { useState } from 'react';
import { Zap, Plus, Trash2, Settings, ToggleLeft, ToggleRight, Shield, Globe, Bell } from 'lucide-react';
import { useTranslation } from 'react-i18next';
import { motion } from 'framer-motion';

interface Extension {
  id: string;
  name: string;
  version: string;
  description: string;
  enabled: boolean;
  icon: string;
  permissions: string[];
  category: string;
}

const DEMO_EXTENSIONS: Extension[] = [
  { id: 'ublock', name: 'uBlock Origin', version: '1.57.2', description: 'An efficient blocker. Easy on CPU and memory.', enabled: true, icon: '🛡️', permissions: ['webRequest', 'tabs', 'storage'], category: 'Privacy' },
  { id: 'bitwarden', name: 'Bitwarden', version: '2024.5.0', description: 'Open source password manager.', enabled: true, icon: '🔐', permissions: ['storage', 'tabs', 'contextMenus'], category: 'Security' },
  { id: 'darkreader', name: 'Dark Reader', version: '4.9.86', description: 'Dark mode for every website.', enabled: false, icon: '🌙', permissions: ['tabs', 'storage', 'webNavigation'], category: 'Appearance' },
  { id: 'grammarly', name: 'Grammarly', version: '14.1119.0', description: 'Writing assistant powered by AI.', enabled: true, icon: '✍️', permissions: ['tabs', 'storage', 'activeTab'], category: 'Productivity' },
  { id: 'honey', name: 'Honey', version: '16.2.0', description: 'Automatically find and apply coupon codes.', enabled: false, icon: '🍯', permissions: ['tabs', 'storage', 'webRequest'], category: 'Shopping' },
  { id: 'lastpass', name: 'LastPass', version: '4.124.0', description: 'Password manager and secure vault.', enabled: false, icon: '🔑', permissions: ['tabs', 'storage', 'contextMenus'], category: 'Security' },
];

export default function ExtensionsPage() {
  const { t } = useTranslation();
  const [extensions, setExtensions] = useState<Extension[]>(DEMO_EXTENSIONS);
  const [filter, setFilter] = useState<'all' | 'enabled' | 'disabled'>('all');

  const toggleExtension = (id: string) => {
    setExtensions(exts => exts.map(e => e.id === id ? { ...e, enabled: !e.enabled } : e));
  };

  const removeExtension = (id: string) => {
    if (confirm('Remove this extension?')) {
      setExtensions(exts => exts.filter(e => e.id !== id));
    }
  };

  const filtered = extensions.filter(e => {
    if (filter === 'enabled') return e.enabled;
    if (filter === 'disabled') return !e.enabled;
    return true;
  });

  return (
    <div className="max-w-3xl mx-auto p-6">
      <div className="flex items-center justify-between mb-6">
        <h1 className="text-2xl font-semibold text-gray-900 dark:text-gray-100 flex items-center gap-2">
          <Zap size={24} className="text-purple-500" />
          {t('extensions.title')}
        </h1>
        <button className="btn-primary gap-2 text-sm">
          <Plus size={14} />
          {t('extensions.install')}
        </button>
      </div>

      {/* Filter tabs */}
      <div className="flex gap-1 mb-6 bg-gray-100 dark:bg-gray-800 rounded-lg p-1">
        {(['all', 'enabled', 'disabled'] as const).map(f => (
          <button
            key={f}
            onClick={() => setFilter(f)}
            className={`flex-1 py-1.5 rounded-md text-sm font-medium transition-colors ${
              filter === f
                ? 'bg-white dark:bg-gray-700 text-gray-900 dark:text-gray-100 shadow-sm'
                : 'text-gray-500 dark:text-gray-400 hover:text-gray-700 dark:hover:text-gray-200'
            }`}
          >
            {f.charAt(0).toUpperCase() + f.slice(1)}
            <span className="ml-1.5 text-xs text-gray-400">
              ({f === 'all' ? extensions.length : f === 'enabled' ? extensions.filter(e => e.enabled).length : extensions.filter(e => !e.enabled).length})
            </span>
          </button>
        ))}
      </div>

      {filtered.length === 0 ? (
        <div className="text-center py-12 text-gray-400">
          <Zap size={40} className="mx-auto mb-3 opacity-30" />
          <p>{t('extensions.noExtensions')}</p>
        </div>
      ) : (
        <div className="space-y-3">
          {filtered.map((ext) => (
            <motion.div
              key={ext.id}
              initial={{ opacity: 0, y: 5 }}
              animate={{ opacity: 1, y: 0 }}
              className={`p-4 rounded-xl border transition-all ${
                ext.enabled
                  ? 'bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700'
                  : 'bg-gray-50 dark:bg-gray-900 border-gray-100 dark:border-gray-800 opacity-60'
              }`}
            >
              <div className="flex items-start gap-3">
                <div className="w-10 h-10 rounded-xl bg-gray-100 dark:bg-gray-700 flex items-center justify-center text-xl flex-shrink-0">
                  {ext.icon}
                </div>
                <div className="flex-1 min-w-0">
                  <div className="flex items-center gap-2">
                    <span className="text-sm font-semibold text-gray-900 dark:text-gray-100">{ext.name}</span>
                    <span className="badge badge-gray text-[10px]">v{ext.version}</span>
                    <span className="badge badge-blue text-[10px]">{ext.category}</span>
                  </div>
                  <p className="text-xs text-gray-500 dark:text-gray-400 mt-0.5">{ext.description}</p>
                  <div className="flex flex-wrap gap-1 mt-2">
                    {ext.permissions.map(p => (
                      <span key={p} className="badge badge-gray text-[10px]">{p}</span>
                    ))}
                  </div>
                </div>
                <div className="flex items-center gap-2 flex-shrink-0">
                  <button className="icon-btn w-7 h-7" title={t('extensions.settings')}>
                    <Settings size={14} />
                  </button>
                  <button
                    onClick={() => toggleExtension(ext.id)}
                    className={`icon-btn w-7 h-7 ${ext.enabled ? 'text-primary-500' : 'text-gray-400'}`}
                    title={ext.enabled ? t('extensions.disable') : t('extensions.enable')}
                  >
                    {ext.enabled ? <ToggleRight size={18} /> : <ToggleLeft size={18} />}
                  </button>
                  <button
                    onClick={() => removeExtension(ext.id)}
                    className="icon-btn w-7 h-7 text-red-400 hover:text-red-600"
                    title={t('extensions.remove')}
                  >
                    <Trash2 size={14} />
                  </button>
                </div>
              </div>
            </motion.div>
          ))}
        </div>
      )}

      {/* Extension store link */}
      <div className="mt-8 p-4 rounded-xl bg-primary-50 dark:bg-primary-900/20 border border-primary-100 dark:border-primary-800">
        <div className="flex items-center gap-3">
          <Globe size={20} className="text-primary-500 flex-shrink-0" />
          <div>
            <div className="text-sm font-medium text-primary-700 dark:text-primary-300">Chrome Web Store Compatible</div>
            <div className="text-xs text-primary-500 dark:text-primary-400 mt-0.5">
              eBrowser supports WebExtensions V3 — compatible with Chrome, Firefox, and Edge extensions.
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
