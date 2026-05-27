import React, { useEffect, useState, useCallback } from 'react';
import { motion } from 'framer-motion';
import { Search, Clock, Bookmark, Globe, ArrowUpRight, Star } from 'lucide-react';
import { useTranslation } from 'react-i18next';
import { historyDB, bookmarkDB } from '@/utils/database';
import { normalizeInput, getFaviconUrl } from '@/utils/url';
import { useBrowserStore } from '@/store/browserStore';

interface Suggestion {
  type: 'history' | 'bookmark' | 'search' | 'url';
  url: string;
  title: string;
  favicon?: string;
  subtitle?: string;
}

interface Props {
  query: string;
  onSelect: (url: string) => void;
  onClose: () => void;
}

export default function AddressBarDropdown({ query, onSelect, onClose }: Props) {
  const { t } = useTranslation();
  const { getSearchUrl, pinnedSites } = useBrowserStore();
  const [suggestions, setSuggestions] = useState<Suggestion[]>([]);
  const [selectedIndex, setSelectedIndex] = useState(0);

  const fetchSuggestions = useCallback(async (q: string) => {
    const results: Suggestion[] = [];

    if (!q.trim()) {
      // Show recent history when empty
      const recent = await historyDB.getRecent(6);
      recent.forEach(h => results.push({
        type: 'history',
        url: h.url,
        title: h.title || h.url,
        favicon: h.favicon || getFaviconUrl(h.url),
        subtitle: `${h.visitCount} visits`,
      }));
    } else {
      // Search history
      const historyResults = await historyDB.search(q);
      historyResults.slice(0, 4).forEach(h => results.push({
        type: 'history',
        url: h.url,
        title: h.title || h.url,
        favicon: h.favicon || getFaviconUrl(h.url),
        subtitle: h.url,
      }));

      // Search bookmarks
      const bookmarkResults = await bookmarkDB.search(q);
      bookmarkResults.slice(0, 3).forEach(b => results.push({
        type: 'bookmark',
        url: b.url,
        title: b.title || b.url,
        favicon: b.favicon || getFaviconUrl(b.url),
        subtitle: b.url,
      }));

      // Add search suggestion
      results.push({
        type: 'search',
        url: getSearchUrl(q),
        title: `Search for "${q}"`,
        subtitle: 'Web Search',
      });

      // If looks like a URL, add direct navigation
      if (q.includes('.') && !q.includes(' ')) {
        const normalized = normalizeInput(q);
        if (!results.find(r => r.url === normalized)) {
          results.unshift({
            type: 'url',
            url: normalized,
            title: normalized,
            favicon: getFaviconUrl(normalized),
            subtitle: 'Navigate to',
          });
        }
      }
    }

    setSuggestions(results.slice(0, 8));
    setSelectedIndex(0);
  }, [getSearchUrl]);

  useEffect(() => {
    const timer = setTimeout(() => fetchSuggestions(query), 100);
    return () => clearTimeout(timer);
  }, [query, fetchSuggestions]);

  useEffect(() => {
    const handler = (e: KeyboardEvent) => {
      if (e.key === 'ArrowDown') {
        e.preventDefault();
        setSelectedIndex(i => Math.min(i + 1, suggestions.length - 1));
      } else if (e.key === 'ArrowUp') {
        e.preventDefault();
        setSelectedIndex(i => Math.max(i - 1, 0));
      } else if (e.key === 'Enter') {
        if (suggestions[selectedIndex]) {
          onSelect(suggestions[selectedIndex].url);
        }
      }
    };
    window.addEventListener('keydown', handler);
    return () => window.removeEventListener('keydown', handler);
  }, [suggestions, selectedIndex, onSelect]);

  if (suggestions.length === 0) return null;

  return (
    <motion.div
      initial={{ opacity: 0, y: -4, scale: 0.98 }}
      animate={{ opacity: 1, y: 0, scale: 1 }}
      exit={{ opacity: 0, y: -4, scale: 0.98 }}
      transition={{ duration: 0.1 }}
      className="absolute top-full left-0 right-0 mt-1 bg-white dark:bg-gray-800 rounded-xl shadow-dropdown border border-gray-200 dark:border-gray-700 overflow-hidden z-dropdown"
    >
      {suggestions.map((suggestion, index) => (
        <button
          key={`${suggestion.type}-${suggestion.url}-${index}`}
          data-dropdown-item
          className={`
            w-full flex items-center gap-3 px-4 py-2.5 text-left transition-colors duration-75
            ${index === selectedIndex
              ? 'bg-primary-50 dark:bg-primary-900/20'
              : 'hover:bg-gray-50 dark:hover:bg-gray-700'
            }
          `}
          onMouseDown={(e) => { e.preventDefault(); onSelect(suggestion.url); }}
          onMouseEnter={() => setSelectedIndex(index)}
        >
          {/* Icon */}
          <div className="flex-shrink-0 w-5 h-5 flex items-center justify-center">
            {suggestion.type === 'search' ? (
              <Search size={14} className="text-gray-400" />
            ) : suggestion.type === 'bookmark' ? (
              <Star size={14} className="text-yellow-500" />
            ) : suggestion.type === 'history' ? (
              suggestion.favicon ? (
                <img src={suggestion.favicon} alt="" className="w-4 h-4 rounded-sm" onError={(e) => { (e.target as HTMLImageElement).style.display = 'none'; }} />
              ) : (
                <Clock size={14} className="text-gray-400" />
              )
            ) : suggestion.favicon ? (
              <img src={suggestion.favicon} alt="" className="w-4 h-4 rounded-sm" onError={(e) => { (e.target as HTMLImageElement).style.display = 'none'; }} />
            ) : (
              <Globe size={14} className="text-gray-400" />
            )}
          </div>

          {/* Content */}
          <div className="flex-1 min-w-0">
            <div className="text-sm font-medium text-gray-900 dark:text-gray-100 truncate">
              {suggestion.title}
            </div>
            {suggestion.subtitle && (
              <div className="text-xs text-gray-400 dark:text-gray-500 truncate">
                {suggestion.subtitle}
              </div>
            )}
          </div>

          {/* Type badge */}
          <div className="flex-shrink-0">
            {suggestion.type === 'bookmark' && (
              <span className="badge badge-yellow text-[10px]">Bookmark</span>
            )}
            {suggestion.type === 'history' && (
              <span className="badge badge-gray text-[10px]">History</span>
            )}
          </div>

          <ArrowUpRight size={12} className="flex-shrink-0 text-gray-300 dark:text-gray-600" />
        </button>
      ))}
    </motion.div>
  );
}
