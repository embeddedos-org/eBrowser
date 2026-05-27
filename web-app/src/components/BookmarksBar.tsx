import React, { useEffect, useState } from 'react';
import { Star, Folder } from 'lucide-react';
import { bookmarkDB, type Bookmark } from '@/utils/database';
import { useBrowserStore } from '@/store/browserStore';

export default function BookmarksBar() {
  const { getActiveTab, navigateTo } = useBrowserStore();
  const [bookmarks, setBookmarks] = useState<Bookmark[]>([]);

  useEffect(() => {
    bookmarkDB.getAll().then(bms => setBookmarks(bms.slice(0, 20)));
  }, []);

  const handleNavigate = (url: string) => {
    const tab = getActiveTab();
    if (tab) navigateTo(url, tab.id);
  };

  if (bookmarks.length === 0) return null;

  return (
    <div className="flex items-center gap-0.5 px-2 h-7 bg-gray-50 dark:bg-gray-850 border-b border-gray-200 dark:border-gray-700 overflow-x-auto no-scrollbar">
      {bookmarks.map(bm => (
        <button
          key={bm.id}
          onClick={() => handleNavigate(bm.url)}
          className="flex items-center gap-1.5 px-2 py-1 rounded-md text-xs text-gray-600 dark:text-gray-400 hover:bg-gray-200 dark:hover:bg-gray-700 hover:text-gray-900 dark:hover:text-gray-100 transition-colors whitespace-nowrap flex-shrink-0"
          title={bm.url}
        >
          {bm.favicon ? (
            <img src={bm.favicon} alt="" className="w-3 h-3 rounded-sm" onError={(e) => { (e.target as HTMLImageElement).style.display = 'none'; }} />
          ) : (
            <Star size={10} className="text-yellow-500" />
          )}
          <span className="max-w-[120px] truncate">{bm.title}</span>
        </button>
      ))}
    </div>
  );
}
