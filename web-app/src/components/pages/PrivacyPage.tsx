import React, { useState } from 'react';
import { Shield, ShieldCheck, ShieldAlert, Eye, EyeOff, Lock, Globe, Fingerprint, Cookie, Wifi, Trash2, BarChart2 } from 'lucide-react';
import { useTranslation } from 'react-i18next';
import { motion } from 'framer-motion';
import { useBrowserStore } from '@/store/browserStore';

export default function PrivacyPage() {
  const { t } = useTranslation();
  const { settings, updateSettings, addToast } = useBrowserStore();
  const [showReport, setShowReport] = useState(false);

  const privacyScore = [
    settings.blockTrackers,
    settings.blockAds,
    settings.enableHTTPSOnly,
    settings.enableDoH,
    settings.enableAntiFingerprint,
    settings.enableGPC,
    settings.enableDNT,
    settings.enablePopupBlocker,
  ].filter(Boolean).length;

  const scorePercent = Math.round((privacyScore / 8) * 100);
  const scoreColor = scorePercent >= 75 ? 'text-green-500' : scorePercent >= 50 ? 'text-yellow-500' : 'text-red-500';
  const scoreBg = scorePercent >= 75 ? 'bg-green-500' : scorePercent >= 50 ? 'bg-yellow-500' : 'bg-red-500';

  const PRIVACY_FEATURES = [
    { key: 'blockTrackers', icon: <Eye size={16} />, label: 'Tracker Blocking', description: 'Block known tracking scripts and pixels from 3rd party domains.' },
    { key: 'blockAds', icon: <EyeOff size={16} />, label: 'Ad Blocking', description: 'Block advertisements across all websites.' },
    { key: 'enableHTTPSOnly', icon: <Lock size={16} />, label: 'HTTPS-Only Mode', description: 'Automatically upgrade HTTP connections to HTTPS.' },
    { key: 'enableDoH', icon: <Globe size={16} />, label: 'DNS over HTTPS', description: 'Encrypt DNS queries to prevent ISP snooping.' },
    { key: 'enableAntiFingerprint', icon: <Fingerprint size={16} />, label: 'Anti-Fingerprinting', description: 'Randomize browser fingerprint to prevent tracking.' },
    { key: 'enableGPC', icon: <Shield size={16} />, label: 'Global Privacy Control', description: 'Signal your privacy preferences to websites (GPC header).' },
    { key: 'enableDNT', icon: <ShieldCheck size={16} />, label: 'Do Not Track', description: 'Send DNT header to request websites not to track you.' },
    { key: 'enablePopupBlocker', icon: <ShieldAlert size={16} />, label: 'Popup Blocker', description: 'Block unwanted popup windows and redirects.' },
  ] as const;

  return (
    <div className="max-w-3xl mx-auto p-6">
      <div className="flex items-center gap-2 mb-6">
        <Shield size={24} className="text-green-500" />
        <h1 className="text-2xl font-semibold text-gray-900 dark:text-gray-100">{t('privacy.title')}</h1>
      </div>

      {/* Privacy Score */}
      <motion.div
        initial={{ opacity: 0, scale: 0.95 }}
        animate={{ opacity: 1, scale: 1 }}
        className="card mb-6"
      >
        <div className="flex items-center justify-between mb-3">
          <h2 className="text-base font-semibold text-gray-900 dark:text-gray-100">Privacy Score</h2>
          <span className={`text-2xl font-bold ${scoreColor}`}>{scorePercent}%</span>
        </div>
        <div className="h-3 bg-gray-100 dark:bg-gray-700 rounded-full overflow-hidden">
          <motion.div
            initial={{ width: 0 }}
            animate={{ width: `${scorePercent}%` }}
            transition={{ duration: 0.8, ease: 'easeOut' }}
            className={`h-full ${scoreBg} rounded-full`}
          />
        </div>
        <div className="flex justify-between text-xs text-gray-400 mt-1">
          <span>Basic</span>
          <span>Standard</span>
          <span>Maximum</span>
        </div>
        <p className="text-sm text-gray-500 dark:text-gray-400 mt-3">
          {scorePercent >= 75
            ? '🛡️ Excellent! Your privacy is well protected.'
            : scorePercent >= 50
            ? '⚠️ Good, but you can improve your privacy further.'
            : '🚨 Your privacy protection is low. Enable more features below.'}
        </p>
      </motion.div>

      {/* Privacy features */}
      <div className="space-y-3 mb-6">
        {PRIVACY_FEATURES.map(({ key, icon, label, description }) => {
          const enabled = settings[key as keyof typeof settings] as boolean;
          return (
            <div
              key={key}
              className={`flex items-center gap-3 p-4 rounded-xl border transition-all ${
                enabled
                  ? 'bg-green-50 dark:bg-green-900/10 border-green-200 dark:border-green-800'
                  : 'bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700'
              }`}
            >
              <div className={`flex-shrink-0 ${enabled ? 'text-green-500' : 'text-gray-400'}`}>{icon}</div>
              <div className="flex-1">
                <div className="text-sm font-medium text-gray-900 dark:text-gray-100">{label}</div>
                <div className="text-xs text-gray-500 dark:text-gray-400">{description}</div>
              </div>
              <button
                onClick={() => updateSettings({ [key]: !enabled })}
                className={`relative inline-flex h-5 w-9 items-center rounded-full transition-colors ${
                  enabled ? 'bg-green-500' : 'bg-gray-300 dark:bg-gray-600'
                }`}
              >
                <span
                  className="inline-block h-3.5 w-3.5 transform rounded-full bg-white shadow-sm transition-transform"
                  style={{ transform: enabled ? 'translateX(18px)' : 'translateX(2px)' }}
                />
              </button>
            </div>
          );
        })}
      </div>

      {/* Clear data */}
      <div className="card">
        <h2 className="text-base font-semibold text-gray-900 dark:text-gray-100 mb-4 flex items-center gap-2">
          <Trash2 size={16} />
          Clear Browsing Data
        </h2>
        <div className="grid grid-cols-2 gap-2">
          {['History', 'Cookies', 'Cache', 'Passwords', 'Form Data', 'Site Data'].map(item => (
            <button
              key={item}
              onClick={() => addToast({ type: 'success', message: `${item} cleared` })}
              className="btn-secondary text-sm justify-start gap-2"
            >
              <Trash2 size={12} />
              Clear {item}
            </button>
          ))}
        </div>
        <button
          onClick={() => addToast({ type: 'success', message: 'All browsing data cleared' })}
          className="btn-danger w-full mt-3 gap-2"
        >
          <Trash2 size={14} />
          Clear All Browsing Data
        </button>
      </div>
    </div>
  );
}
