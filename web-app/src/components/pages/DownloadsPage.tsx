import React, { useState, useEffect } from 'react';
import { Download, Trash2, FolderOpen, X, CheckCircle, AlertCircle, Pause, Play, RefreshCw } from 'lucide-react';
import { useTranslation } from 'react-i18next';
import { motion } from 'framer-motion';
import { downloadDB, type DownloadItem } from '@/utils/database';
import { getReadableFileSize } from '@/utils/url';

export default function DownloadsPage() {
  const { t } = useTranslation();
  const [downloads, setDownloads] = useState<DownloadItem[]>([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    downloadDB.getAll().then(d => { setDownloads(d); setLoading(false); });
  }, []);

  const handleClearCompleted = async () => {
    await downloadDB.clearCompleted();
    setDownloads(d => d.filter(item => item.status === 'downloading' || item.status === 'pending'));
  };

  const StatusIcon = ({ status }: { status: DownloadItem['status'] }) => {
    switch (status) {
      case 'completed': return <CheckCircle size={16} className="text-green-500" />;
      case 'failed': return <AlertCircle size={16} className="text-red-500" />;
      case 'downloading': return <div className="w-4 h-4 border-2 border-primary-500 border-t-transparent rounded-full animate-spin" />;
      case 'paused': return <Pause size={16} className="text-yellow-500" />;
      default: return <Download size={16} className="text-gray-400" />;
    }
  };

  return (
    <div className="max-w-3xl mx-auto p-6">
      <div className="flex items-center justify-between mb-6">
        <h1 className="text-2xl font-semibold text-gray-900 dark:text-gray-100 flex items-center gap-2">
          <Download size={24} className="text-primary-500" />
          {t('downloads.title')}
        </h1>
        <button onClick={handleClearCompleted} className="btn-secondary gap-2 text-sm">
          <Trash2 size={14} />
          {t('downloads.clearAll')}
        </button>
      </div>

      {loading ? (
        <div className="flex items-center justify-center py-12 text-gray-400">
          <div className="animate-spin w-6 h-6 border-2 border-primary-500 border-t-transparent rounded-full mr-3" />
          Loading...
        </div>
      ) : downloads.length === 0 ? (
        <div className="text-center py-12 text-gray-400">
          <Download size={40} className="mx-auto mb-3 opacity-30" />
          <p>{t('downloads.noDownloads')}</p>
        </div>
      ) : (
        <div className="space-y-2">
          {downloads.map((item) => (
            <motion.div
              key={item.id}
              initial={{ opacity: 0, y: 5 }}
              animate={{ opacity: 1, y: 0 }}
              className="flex items-center gap-3 p-4 rounded-xl bg-white dark:bg-gray-800 border border-gray-100 dark:border-gray-700"
            >
              <StatusIcon status={item.status} />
              <div className="flex-1 min-w-0">
                <div className="text-sm font-medium text-gray-900 dark:text-gray-100 truncate">{item.filename}</div>
                <div className="text-xs text-gray-400 truncate">{item.url}</div>
                {item.status === 'downloading' && item.size && (
                  <div className="mt-1">
                    <div className="h-1 bg-gray-200 dark:bg-gray-700 rounded-full overflow-hidden">
                      <div
                        className="h-full bg-primary-500 rounded-full transition-all"
                        style={{ width: `${((item.downloadedBytes ?? 0) / item.size) * 100}%` }}
                      />
                    </div>
                    <div className="text-xs text-gray-400 mt-0.5">
                      {getReadableFileSize(item.downloadedBytes ?? 0)} / {getReadableFileSize(item.size)}
                    </div>
                  </div>
                )}
                {item.size && item.status === 'completed' && (
                  <div className="text-xs text-gray-400">{getReadableFileSize(item.size)}</div>
                )}
              </div>
              <div className="flex items-center gap-1 flex-shrink-0">
                {item.status === 'completed' && (
                  <button className="icon-btn w-7 h-7" title={t('downloads.openFolder')}>
                    <FolderOpen size={14} />
                  </button>
                )}
                {item.status === 'downloading' && (
                  <button className="icon-btn w-7 h-7" title={t('downloads.pause')}>
                    <Pause size={14} />
                  </button>
                )}
                {item.status === 'failed' && (
                  <button className="icon-btn w-7 h-7" title={t('downloads.retry')}>
                    <RefreshCw size={14} />
                  </button>
                )}
                <button
                  onClick={() => item.id && downloadDB.delete(item.id).then(() => setDownloads(d => d.filter(i => i.id !== item.id)))}
                  className="icon-btn w-7 h-7 text-red-400 hover:text-red-600"
                  title={t('downloads.remove')}
                >
                  <X size={14} />
                </button>
              </div>
            </motion.div>
          ))}
        </div>
      )}
    </div>
  );
}
