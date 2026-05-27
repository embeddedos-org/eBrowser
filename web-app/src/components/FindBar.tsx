import React, { useRef, useEffect } from 'react';
import { Search, ChevronUp, ChevronDown, X } from 'lucide-react';
import { useBrowserStore } from '@/store/browserStore';

export default function FindBar() {
  const { findQuery, findResults, findCurrentIndex, setFindQuery, toggleFindBar } = useBrowserStore();
  const inputRef = useRef<HTMLInputElement>(null);

  useEffect(() => { inputRef.current?.focus(); }, []);

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === 'Escape') toggleFindBar();
    if (e.key === 'Enter') {
      // In a real browser this would trigger find-next
    }
  };

  return (
    <div className="flex items-center gap-2 px-3 py-1.5 bg-white dark:bg-gray-800 border-b border-gray-200 dark:border-gray-700 shadow-sm">
      <Search size={14} className="text-gray-400 flex-shrink-0" />
      <input
        ref={inputRef}
        type="text"
        value={findQuery}
        onChange={(e) => setFindQuery(e.target.value)}
        onKeyDown={handleKeyDown}
        placeholder="Find in page..."
        className="flex-1 bg-transparent outline-none text-sm text-gray-900 dark:text-gray-100 placeholder-gray-400"
      />
      {findQuery && (
        <span className="text-xs text-gray-400">
          {findResults > 0 ? `${findCurrentIndex + 1}/${findResults}` : 'No results'}
        </span>
      )}
      <div className="flex items-center gap-0.5">
        <button className="icon-btn w-6 h-6" title="Previous (Shift+Enter)">
          <ChevronUp size={14} />
        </button>
        <button className="icon-btn w-6 h-6" title="Next (Enter)">
          <ChevronDown size={14} />
        </button>
        <button onClick={toggleFindBar} className="icon-btn w-6 h-6" title="Close (Esc)">
          <X size={14} />
        </button>
      </div>
    </div>
  );
}
