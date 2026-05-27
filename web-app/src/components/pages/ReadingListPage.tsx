import React, { useState, useEffect } from 'react';
import { BookOpen, Check, Trash2, Clock, ExternalLink } from 'lucide-react';
import { useTranslation } from 'react-i18next';
import { motion } from 'framer-motion';
import { readingListDB, type ReadingListItem } from '@/utils/database';
import { useBrowserStore } from '@/store/browserStore';
import { format } from 'date-fns';

export default function ReadingListPage() {
  const { t } = useTranslation();
  const store = useBrowserStore();
  const [items, setItems] = useState<ReadingListItem[]>([]);
  const [filter, setFilter] = useState<'all' | 'unread' | 'read'>('all');

  useEffect(() => { readingListDB.getAll().then(setItems); }, []);

  const handleMarkRead = async (id: number) => {
    await readingListDB.markRead(id);
    setItems(prev => prev.map(i => i.id === id ? { ...i, isRead: true, readAt: new Date() } : i));
  };

  const handleDelete = async (id: number) => {
    await readingListDB.delete(id);
    setItems(prev => prev.filter(i => i.id !== id));
  };

  const filtered = items.filter(i => {
    if (filter === 'unread') return !i.isRead;
    if (filter === 'read') return i.isRead;
    return true;
  });

  return (
    <div className="max-w-3xl mx-auto p-6">
      <div className="flex items-center gap-2 mb-6">
        <BookOpen size={24} className="text-primary-500" />
        <h1 className="text-2xl font-semibold text-gray-900 dark:text-gray-100">Reading List</h1>
      </div>

      <div className="flex gap-1 mb-6 bg-gray-100 dark:bg-gray-800 rounded-lg p-1">
        {(['all', 'unread', 'read'] as const).map(f => (
          <button key={f} onClick={() => setFilter(f)}
            className={`flex-1 py-1.5 rounded-md text-sm font-medium transition-colors ${filter === f ? 'bg-white dark:bg-gray-700 text-gray-900 dark:text-gray-100 shadow-sm' : 'text-gray-500'}`}
          >
            {f.charAt(0).toUpperCase() + f.slice(1)}
          </button>
        ))}
      </div>

      {filtered.length === 0 ? (
        <div className="text-center py-12 text-gray-400">
          <BookOpen size={40} className="mx-auto mb-3 opacity-30" />
          <p>No items in reading list</p>
          <p className="text-xs mt-1">Save articles to read later by clicking "Add to Reading List"</p>
        </div>
      ) : (
        <div className="space-y-3">
          {filtered.map(item => (
            <motion.div key={item.id} initial={{ opacity: 0 }} animate={{ opacity: 1 }}
              className={`p-4 rounded-xl border ${item.isRead ? 'opacity-60 bg-gray-50 dark:bg-gray-900 border-gray-100 dark:border-gray-800' : 'bg-white dark:bg-gray-800 border-gray-200 dark:border-gray-700'}`}
            >
              <div className="flex items-start gap-3">
                {item.favicon && <img src={item.favicon} alt="" className="w-5 h-5 mt-0.5 rounded-sm flex-shrink-0" />}
                <div className="flex-1 min-w-0">
                  <div className="text-sm font-medium text-gray-900 dark:text-gray-100 truncate">{item.title}</div>
                  {item.excerpt && <div className="text-xs text-gray-500 dark:text-gray-400 mt-0.5 line-clamp-2">{item.excerpt}</div>}
                  <div className="flex items-center gap-3 mt-1 text-xs text-gray-400">
                    <span className="flex items-center gap-1"><Clock size={10} />{format(new Date(item.addedAt), 'MMM d')}</span>
                    {item.estimatedReadTime && <span>~{item.estimatedReadTime} min read</span>}
                    {item.isRead && <span className="text-green-500 flex items-center gap-1"><Check size={10} />Read</span>}
                  </div>
                </div>
                <div className="flex items-center gap-1 flex-shrink-0">
                  <button onClick={() => { const t = store.getActiveTab(); if (t) store.navigateTo(item.url, t.id); }} className="icon-btn w-7 h-7"><ExternalLink size={12} /></button>
                  {!item.isRead && <button onClick={() => item.id && handleMarkRead(item.id)} className="icon-btn w-7 h-7 text-green-500"><Check size={12} /></button>}
                  <button onClick={() => item.id && handleDelete(item.id)} className="icon-btn w-7 h-7 text-red-400"><Trash2 size={12} /></button>
                </div>
              </div>
            </motion.div>
          ))}
        </div>
      )}
    </div>
  );
}
