import React, { useState, useEffect } from 'react';
import { Key, Eye, EyeOff, Trash2, Copy, Search, Shield, Lock } from 'lucide-react';
import { motion } from 'framer-motion';
import { passwordDB, type PasswordEntry } from '@/utils/database';
import { useBrowserStore } from '@/store/browserStore';
import { format } from 'date-fns';

export default function PasswordsPage() {
  const store = useBrowserStore();
  const [passwords, setPasswords] = useState<PasswordEntry[]>([]);
  const [query, setQuery] = useState('');
  const [showPasswords, setShowPasswords] = useState<Set<number>>(new Set());
  const [masterUnlocked, setMasterUnlocked] = useState(false);
  const [masterPin, setMasterPin] = useState('');

  useEffect(() => {
    if (masterUnlocked) {
      passwordDB.getAll().then(setPasswords);
    }
  }, [masterUnlocked]);

  const handleUnlock = () => {
    // In production, this would verify against a stored hash
    if (masterPin.length >= 4) {
      setMasterUnlocked(true);
    } else {
      store.addToast({ type: 'error', message: 'PIN must be at least 4 digits' });
    }
  };

  const toggleShow = (id: number) => {
    setShowPasswords(prev => {
      const next = new Set(prev);
      if (next.has(id)) next.delete(id); else next.add(id);
      return next;
    });
  };

  const handleCopy = (text: string, label: string) => {
    navigator.clipboard.writeText(text).then(() => {
      store.addToast({ type: 'success', message: `${label} copied to clipboard` });
    });
  };

  const filtered = passwords.filter(p =>
    p.url.toLowerCase().includes(query.toLowerCase()) ||
    p.username.toLowerCase().includes(query.toLowerCase())
  );

  if (!masterUnlocked) {
    return (
      <div className="flex items-center justify-center h-full">
        <div className="max-w-sm w-full p-8 text-center">
          <div className="w-16 h-16 rounded-full bg-primary-100 dark:bg-primary-900/20 flex items-center justify-center mx-auto mb-4">
            <Lock size={28} className="text-primary-500" />
          </div>
          <h2 className="text-xl font-semibold text-gray-900 dark:text-gray-100 mb-2">Password Vault</h2>
          <p className="text-sm text-gray-500 dark:text-gray-400 mb-6">Enter your master PIN to access saved passwords</p>
          <input
            type="password"
            value={masterPin}
            onChange={(e) => setMasterPin(e.target.value)}
            onKeyDown={(e) => e.key === 'Enter' && handleUnlock()}
            placeholder="Enter PIN"
            className="input text-center text-lg tracking-widest mb-4"
            maxLength={8}
          />
          <button onClick={handleUnlock} className="btn-primary w-full gap-2">
            <Key size={14} />
            Unlock Vault
          </button>
        </div>
      </div>
    );
  }

  return (
    <div className="max-w-3xl mx-auto p-6">
      <div className="flex items-center justify-between mb-6">
        <h1 className="text-2xl font-semibold text-gray-900 dark:text-gray-100 flex items-center gap-2">
          <Key size={24} className="text-primary-500" />
          Saved Passwords
        </h1>
        <div className="flex items-center gap-2">
          <span className="badge badge-green flex items-center gap-1"><Shield size={10} />Encrypted</span>
          <button onClick={() => setMasterUnlocked(false)} className="btn-secondary text-sm gap-2">
            <Lock size={12} />
            Lock
          </button>
        </div>
      </div>

      <div className="relative mb-6">
        <Search size={16} className="absolute left-3 top-1/2 -translate-y-1/2 text-gray-400" />
        <input type="text" value={query} onChange={(e) => setQuery(e.target.value)} placeholder="Search passwords..." className="input pl-9" />
      </div>

      {filtered.length === 0 ? (
        <div className="text-center py-12 text-gray-400">
          <Key size={40} className="mx-auto mb-3 opacity-30" />
          <p>No saved passwords</p>
        </div>
      ) : (
        <div className="space-y-2">
          {filtered.map(entry => (
            <motion.div key={entry.id} initial={{ opacity: 0 }} animate={{ opacity: 1 }}
              className="flex items-center gap-3 p-4 rounded-xl bg-white dark:bg-gray-800 border border-gray-200 dark:border-gray-700"
            >
              <div className="w-8 h-8 rounded-lg bg-gray-100 dark:bg-gray-700 flex items-center justify-center flex-shrink-0">
                <Key size={14} className="text-gray-400" />
              </div>
              <div className="flex-1 min-w-0">
                <div className="text-sm font-medium text-gray-900 dark:text-gray-100 truncate">{entry.url}</div>
                <div className="text-xs text-gray-400">{entry.username}</div>
                <div className="text-xs text-gray-300 dark:text-gray-600 mt-0.5">
                  Updated {format(new Date(entry.updatedAt), 'MMM d, yyyy')}
                </div>
              </div>
              <div className="flex items-center gap-1 flex-shrink-0">
                <button onClick={() => handleCopy(entry.username, 'Username')} className="icon-btn w-7 h-7" title="Copy username">
                  <Copy size={12} />
                </button>
                <button onClick={() => entry.id && toggleShow(entry.id)} className="icon-btn w-7 h-7" title="Show/hide password">
                  {entry.id && showPasswords.has(entry.id) ? <EyeOff size={12} /> : <Eye size={12} />}
                </button>
                <button onClick={() => entry.id && passwordDB.delete(entry.id).then(() => setPasswords(p => p.filter(e => e.id !== entry.id)))} className="icon-btn w-7 h-7 text-red-400">
                  <Trash2 size={12} />
                </button>
              </div>
            </motion.div>
          ))}
        </div>
      )}
    </div>
  );
}
