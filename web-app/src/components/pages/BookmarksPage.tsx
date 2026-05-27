import React, { useState, useEffect } from 'react';
import { Star, Search, FolderPlus, Trash2, Edit2, ExternalLink, X, Folder, Tag } from 'lucide-react';
import { useTranslation } from 'react-i18next';
import { motion } from 'framer-motion';
import { bookmarkDB, type Bookmark } from '@/utils/database';
import { useBrowserStore } from '@/store/browserStore';

export default function BookmarksPage() {
  const { t } = useTranslation();
  const store = useBrowserStore();
  const [bookmarks, setBookmarks] = useState<Bookmark[]>([]);
  const [query, setQuery] = useState('');
  const [loading, setLoading] = useState(true);
  const [editingId, setEditingId] = useState<number | null>(null);

  const loadBookmarks = async () => {
    setLoading(true);
    const data = query ? await bookmarkDB.search(query) : await bookmarkDB.getAll();
    setBookmarks(data);
    setLoading(false);
  };

  useEffect(() => { loadBookmarks(); }, [query]);

  const handleDelete = async (id: number) => {
    await bookmarkDB.delete(id);
    setBookmarks(b => b.filter(bm => bm.id !== id));
    store.addToast({ type: 'info', message: t('bookmarks.bookmarkRemoved') });
  };

  const handleNavigate = (url: string) => {
    const tab = store.getActiveTab();
    if (tab) store.navigateTo(url, tab.id);
  };

  return (
    <div className="max-w-3xl mx-auto p-6">
      <div className="flex items-center justify-between mb-6">
        <h1 className="text-2xl font-semibold text-gray-900 dark:text-gray-100 flex items-center gap-2">
          <Star size={24} className="text-yellow-500" />
          {t('bookmarks.title')}
        </h1>
        <div className="flex gap-2">
          <button className="btn-secondary gap-2 text-sm">
            <FolderPlus size={14} />
            {t('bookmarks.addFolder')}
          </button>
        </div>
      </div>

      <div className="relative mb-6">
        <Search size={16} className="absolute left-3 top-1/2 -translate-y-1/2 text-gray-400" />
        <input
          type="text"
          value={query}
          onChange={(e) => setQuery(e.target.value)}
          placeholder={t('bookmarks.searchBookmarks')}
          className="input pl-9"
        />
        {query && (
          <button onClick={() => setQuery('')} className="absolute right-3 top-1/2 -translate-y-1/2 text-gray-400">
            <X size={14} />
          </button>
        )}
      </div>

      {loading ? (
        <div className="flex items-center justify-center py-12 text-gray-400">
          <div className="animate-spin w-6 h-6 border-2 border-primary-500 border-t-transparent rounded-full mr-3" />
          Loading...
        </div>
      ) : bookmarks.length === 0 ? (
        <div className="text-center py-12 text-gray-400">
          <Star size={40} className="mx-auto mb-3 opacity-30" />
          <p>{t('bookmarks.noBookmarks')}</p>
        </div>
      ) : (
        <div className="grid grid-cols-1 gap-2">
          {bookmarks.map((bm) => (
            <motion.div
              key={bm.id}
              initial={{ opacity: 0, y: 5 }}
              animate={{ opacity: 1, y: 0 }}
              className="group flex items-center gap-3 p-3 rounded-xl bg-white dark:bg-gray-800 border border-gray-100 dark:border-gray-700 hover:border-primary-200 dark:hover:border-primary-800 transition-all"
            >
              <div className="w-8 h-8 rounded-lg bg-gray-100 dark:bg-gray-700 flex items-center justify-center flex-shrink-0 overflow-hidden">
                {bm.favicon ? (
                  <img src={bm.favicon} alt="" className="w-5 h-5" onError={(e) => { (e.target as HTMLImageElement).style.display = 'none'; }} />
                ) : (
                  <Star size={14} className="text-yellow-500" />
                )}
              </div>
              <button onClick={() => handleNavigate(bm.url)} className="flex-1 min-w-0 text-left">
                <div className="text-sm font-medium text-gray-900 dark:text-gray-100 truncate">{bm.title}</div>
                <div className="text-xs text-gray-400 truncate">{bm.url}</div>
              </button>
              {bm.tags && bm.tags.length > 0 && (
                <div className="flex gap-1 flex-shrink-0">
                  {bm.tags.slice(0, 2).map(tag => (
                    <span key={tag} className="badge badge-blue text-[10px]">{tag}</span>
                  ))}
                </div>
              )}
              <div className="flex items-center gap-1 opacity-0 group-hover:opacity-100 transition-opacity flex-shrink-0">
                <button onClick={() => window.open(bm.url, '_blank')} className="icon-btn w-6 h-6" title="Open in new tab">
                  <ExternalLink size={12} />
                </button>
                <button onClick={() => setEditingId(bm.id ?? null)} className="icon-btn w-6 h-6" title={t('bookmarks.editBookmark')}>
                  <Edit2 size={12} />
                </button>
                <button onClick={() => bm.id && handleDelete(bm.id)} className="icon-btn w-6 h-6 text-red-400 hover:text-red-600" title={t('bookmarks.deleteBookmark')}>
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
