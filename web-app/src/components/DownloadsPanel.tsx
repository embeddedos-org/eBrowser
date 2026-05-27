import React, { useState, useEffect } from 'react';
import { Download, X, CheckCircle, AlertCircle, FolderOpen, ExternalLink } from 'lucide-react';
import { useBrowserStore } from '@/store/browserStore';
import { downloadDB, type DownloadItem } from '@/utils/database';
import { getReadableFileSize } from '@/utils/url';

export default function DownloadsPanel() {
  const { toggleDownloadsPanel, navigateTo, getActiveTab } = useBrowserStore();
  const [downloads, setDownloads] = useState<DownloadItem[]>([]);

  useEffect(() => {
    downloadDB.getAll().then(d => setDownloads(d.slice(0, 10)));
  }, []);

  return (
    <div className="border-t border-gray-200 dark:border-gray-700 bg-white dark:bg-gray-800 max-h-48 overflow-y-auto">
      <div className="flex items-center justify-between px-4 py-2 border-b border-gray-100 dark:border-gray-700">
        <span className="text-sm font-semibold text-gray-900 dark:text-gray-100 flex items-center gap-2">
          <Download size={14} />
          Downloads
        </span>
        <div className="flex items-center gap-2">
          <button
            onClick={() => { const t = getActiveTab(); if (t) navigateTo('ebrowser://downloads', t.id); toggleDownloadsPanel(); }}
            className="text-xs text-primary-500 hover:text-primary-600"
          >
            View all
          </button>
          <button onClick={toggleDownloadsPanel} className="icon-btn w-5 h-5">
            <X size={12} />
          </button>
        </div>
      </div>
      {downloads.length === 0 ? (
        <div className="text-center py-4 text-gray-400 text-sm">No recent downloads</div>
      ) : (
        <div className="divide-y divide-gray-100 dark:divide-gray-700">
          {downloads.map(item => (
            <div key={item.id} className="flex items-center gap-3 px-4 py-2">
              {item.status === 'completed' ? (
                <CheckCircle size={14} className="text-green-500 flex-shrink-0" />
              ) : item.status === 'failed' ? (
                <AlertCircle size={14} className="text-red-500 flex-shrink-0" />
              ) : (
                <div className="w-3.5 h-3.5 border-2 border-primary-500 border-t-transparent rounded-full animate-spin flex-shrink-0" />
              )}
              <div className="flex-1 min-w-0">
                <div className="text-xs font-medium text-gray-900 dark:text-gray-100 truncate">{item.filename}</div>
                {item.size && <div className="text-xs text-gray-400">{getReadableFileSize(item.size)}</div>}
              </div>
              {item.status === 'completed' && (
                <button className="icon-btn w-5 h-5 flex-shrink-0" title="Open folder">
                  <FolderOpen size={11} />
                </button>
              )}
            </div>
          ))}
        </div>
      )}
    </div>
  );
}
