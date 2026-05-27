import React from 'react';
import { Shield, Zap, Globe, Lock, Cpu, Star, Github, ExternalLink } from 'lucide-react';
import { motion } from 'framer-motion';

const FEATURES = [
  { icon: <Shield size={20} className="text-green-500" />, title: 'Privacy First', description: 'Built-in tracker blocking, anti-fingerprinting, and DoH encryption.' },
  { icon: <Zap size={20} className="text-yellow-500" />, title: 'Lightning Fast', description: 'Optimized rendering engine with GPU acceleration and smart caching.' },
  { icon: <Globe size={20} className="text-blue-500" />, title: '12 Languages', description: 'Full i18n support for English, Spanish, Chinese, Hindi, French, Arabic, and more.' },
  { icon: <Lock size={20} className="text-purple-500" />, title: 'Security Fortress', description: 'TLS 1.3, HTTPS-only mode, sandbox isolation, and memory safety.' },
  { icon: <Cpu size={20} className="text-red-500" />, title: 'Extensions', description: 'WebExtensions V3 compatible — works with Chrome and Firefox extensions.' },
  { icon: <Star size={20} className="text-orange-500" />, title: 'Cross-Platform', description: 'Web app, browser extension, Android, and iOS — one codebase.' },
];

const COMPETITORS = [
  { name: 'Chrome', memory: '300MB/tab', privacy: '⭐⭐', security: '⭐⭐⭐⭐' },
  { name: 'Firefox', memory: '200MB/tab', privacy: '⭐⭐⭐', security: '⭐⭐⭐⭐' },
  { name: 'Brave', memory: '250MB/tab', privacy: '⭐⭐⭐⭐', security: '⭐⭐⭐⭐' },
  { name: 'Tor', memory: '150MB/tab', privacy: '⭐⭐⭐⭐⭐', security: '⭐⭐⭐' },
  { name: 'eBrowser', memory: '<5MB/tab', privacy: '⭐⭐⭐⭐⭐', security: '⭐⭐⭐⭐⭐' },
];

export default function AboutPage() {
  return (
    <div className="max-w-4xl mx-auto p-8">
      {/* Hero */}
      <motion.div initial={{ opacity: 0, y: -20 }} animate={{ opacity: 1, y: 0 }} className="text-center mb-12">
        <div className="w-20 h-20 rounded-3xl bg-gradient-to-br from-primary-500 to-purple-600 flex items-center justify-center text-white text-3xl font-bold shadow-xl mx-auto mb-4">
          eB
        </div>
        <h1 className="text-4xl font-bold text-gray-900 dark:text-gray-100 mb-2">eBrowser</h1>
        <p className="text-xl text-gray-500 dark:text-gray-400">The World's Most Powerful Browser</p>
        <div className="flex items-center justify-center gap-3 mt-4">
          <span className="badge badge-blue">v2.0.0</span>
          <span className="badge badge-green">MIT License</span>
          <span className="badge badge-gray">WebExtensions V3</span>
        </div>
      </motion.div>

      {/* Features grid */}
      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4 mb-12">
        {FEATURES.map((f, i) => (
          <motion.div
            key={f.title}
            initial={{ opacity: 0, y: 20 }}
            animate={{ opacity: 1, y: 0 }}
            transition={{ delay: i * 0.05 }}
            className="card"
          >
            <div className="flex items-center gap-2 mb-2">{f.icon}<span className="font-semibold text-gray-900 dark:text-gray-100">{f.title}</span></div>
            <p className="text-sm text-gray-500 dark:text-gray-400">{f.description}</p>
          </motion.div>
        ))}
      </div>

      {/* Comparison table */}
      <div className="mb-12">
        <h2 className="text-xl font-semibold text-gray-900 dark:text-gray-100 mb-4">How We Compare</h2>
        <div className="overflow-x-auto">
          <table className="w-full text-sm">
            <thead>
              <tr className="border-b border-gray-200 dark:border-gray-700">
                <th className="text-left py-2 px-3 text-gray-500 dark:text-gray-400 font-medium">Browser</th>
                <th className="text-left py-2 px-3 text-gray-500 dark:text-gray-400 font-medium">Memory/Tab</th>
                <th className="text-left py-2 px-3 text-gray-500 dark:text-gray-400 font-medium">Privacy</th>
                <th className="text-left py-2 px-3 text-gray-500 dark:text-gray-400 font-medium">Security</th>
              </tr>
            </thead>
            <tbody>
              {COMPETITORS.map((c) => (
                <tr
                  key={c.name}
                  className={`border-b border-gray-100 dark:border-gray-800 ${c.name === 'eBrowser' ? 'bg-primary-50 dark:bg-primary-900/20 font-semibold' : ''}`}
                >
                  <td className="py-2 px-3 text-gray-900 dark:text-gray-100">{c.name === 'eBrowser' ? '🏆 ' : ''}{c.name}</td>
                  <td className="py-2 px-3 text-gray-700 dark:text-gray-300">{c.memory}</td>
                  <td className="py-2 px-3">{c.privacy}</td>
                  <td className="py-2 px-3">{c.security}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </div>

      {/* Links */}
      <div className="flex flex-wrap gap-3">
        <a href="https://github.com/embeddedos-org/eBrowser" target="_blank" rel="noopener noreferrer" className="btn-secondary gap-2">
          <Github size={14} />
          GitHub Repository
        </a>
        <a href="https://embeddedos-org.github.io/eBrowser/" target="_blank" rel="noopener noreferrer" className="btn-secondary gap-2">
          <ExternalLink size={14} />
          Documentation
        </a>
      </div>
    </div>
  );
}
