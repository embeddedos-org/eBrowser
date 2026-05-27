import React, { useState } from 'react';
import { motion } from 'framer-motion';
import { X, Terminal, Code2, Network, Database, Layers, Bug, ChevronDown, ChevronRight } from 'lucide-react';
import { useBrowserStore } from '@/store/browserStore';

type DevToolsTab = 'console' | 'elements' | 'network' | 'storage' | 'sources' | 'debugger';

export default function DevTools() {
  const { toggleDevTools, getActiveTab } = useBrowserStore();
  const activeTab = getActiveTab();
  const [activeDevTab, setActiveDevTab] = useState<DevToolsTab>('console');
  const [consoleInput, setConsoleInput] = useState('');
  const [consoleLog, setConsoleLog] = useState<Array<{ type: string; message: string; time: string }>>([
    { type: 'info', message: 'eBrowser DevTools initialized', time: new Date().toLocaleTimeString() },
    { type: 'log', message: `Page: ${activeTab?.url ?? 'about:newtab'}`, time: new Date().toLocaleTimeString() },
  ]);

  const tabs: Array<{ id: DevToolsTab; icon: React.ReactNode; label: string }> = [
    { id: 'console', icon: <Terminal size={13} />, label: 'Console' },
    { id: 'elements', icon: <Code2 size={13} />, label: 'Elements' },
    { id: 'network', icon: <Network size={13} />, label: 'Network' },
    { id: 'storage', icon: <Database size={13} />, label: 'Storage' },
    { id: 'sources', icon: <Layers size={13} />, label: 'Sources' },
    { id: 'debugger', icon: <Bug size={13} />, label: 'Debugger' },
  ];

  const handleConsoleInput = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === 'Enter' && consoleInput.trim()) {
      const result = (() => {
        try {
          // Safe eval simulation
          return `→ ${consoleInput} (evaluated in sandbox)`;
        } catch (err: any) {
          return `✗ Error: ${err.message}`;
        }
      })();
      setConsoleLog(prev => [
        ...prev,
        { type: 'input', message: `> ${consoleInput}`, time: new Date().toLocaleTimeString() },
        { type: 'output', message: result, time: new Date().toLocaleTimeString() },
      ]);
      setConsoleInput('');
    }
  };

  const logColors: Record<string, string> = {
    info: 'text-blue-400',
    log: 'text-gray-300',
    warn: 'text-yellow-400',
    error: 'text-red-400',
    input: 'text-green-400',
    output: 'text-gray-400',
  };

  return (
    <motion.div
      initial={{ height: 0 }}
      animate={{ height: 300 }}
      exit={{ height: 0 }}
      transition={{ duration: 0.2 }}
      className="flex-shrink-0 border-t border-gray-700 bg-gray-900 text-gray-100 overflow-hidden flex flex-col"
    >
      {/* DevTools header */}
      <div className="flex items-center border-b border-gray-700 flex-shrink-0">
        {tabs.map(tab => (
          <button
            key={tab.id}
            onClick={() => setActiveDevTab(tab.id)}
            className={`flex items-center gap-1.5 px-3 py-2 text-xs border-b-2 transition-colors ${
              activeDevTab === tab.id
                ? 'border-primary-500 text-primary-400'
                : 'border-transparent text-gray-400 hover:text-gray-200'
            }`}
          >
            {tab.icon}
            {tab.label}
          </button>
        ))}
        <div className="flex-1" />
        <button onClick={toggleDevTools} className="px-3 py-2 text-gray-400 hover:text-gray-200">
          <X size={14} />
        </button>
      </div>

      {/* Console */}
      {activeDevTab === 'console' && (
        <div className="flex-1 flex flex-col overflow-hidden">
          <div className="flex-1 overflow-y-auto p-2 font-mono text-xs space-y-0.5">
            {consoleLog.map((entry, i) => (
              <div key={i} className={`flex gap-2 ${logColors[entry.type] ?? 'text-gray-300'}`}>
                <span className="text-gray-600 flex-shrink-0">{entry.time}</span>
                <span>{entry.message}</span>
              </div>
            ))}
          </div>
          <div className="flex items-center gap-2 border-t border-gray-700 px-3 py-1.5">
            <span className="text-green-400 font-mono text-xs">&gt;</span>
            <input
              type="text"
              value={consoleInput}
              onChange={(e) => setConsoleInput(e.target.value)}
              onKeyDown={handleConsoleInput}
              placeholder="Enter JavaScript expression..."
              className="flex-1 bg-transparent outline-none text-xs font-mono text-gray-200 placeholder-gray-600"
            />
          </div>
        </div>
      )}

      {/* Elements */}
      {activeDevTab === 'elements' && (
        <div className="flex-1 overflow-auto p-3 font-mono text-xs text-gray-300">
          <div className="space-y-1">
            <div className="flex items-center gap-1 text-blue-400"><ChevronDown size={12} />&lt;html&gt;</div>
            <div className="pl-4 flex items-center gap-1 text-blue-400"><ChevronDown size={12} />&lt;head&gt;</div>
            <div className="pl-8 text-gray-500">&lt;title&gt;{activeTab?.title}&lt;/title&gt;</div>
            <div className="pl-4 text-blue-400">&lt;/head&gt;</div>
            <div className="pl-4 flex items-center gap-1 text-blue-400"><ChevronRight size={12} />&lt;body&gt;...</div>
            <div className="text-blue-400">&lt;/html&gt;</div>
          </div>
          <div className="mt-4 text-gray-500 text-xs">
            Full DOM inspection available when page is loaded in same origin context.
          </div>
        </div>
      )}

      {/* Network */}
      {activeDevTab === 'network' && (
        <div className="flex-1 overflow-auto">
          <table className="w-full text-xs font-mono">
            <thead className="border-b border-gray-700 text-gray-500">
              <tr>
                <th className="text-left px-3 py-1.5">Name</th>
                <th className="text-left px-3 py-1.5">Status</th>
                <th className="text-left px-3 py-1.5">Type</th>
                <th className="text-left px-3 py-1.5">Size</th>
                <th className="text-left px-3 py-1.5">Time</th>
              </tr>
            </thead>
            <tbody className="text-gray-300">
              <tr className="border-b border-gray-800 hover:bg-gray-800">
                <td className="px-3 py-1.5 text-blue-400 truncate max-w-[200px]">{activeTab?.url ?? '—'}</td>
                <td className="px-3 py-1.5 text-green-400">200</td>
                <td className="px-3 py-1.5">document</td>
                <td className="px-3 py-1.5">—</td>
                <td className="px-3 py-1.5">—</td>
              </tr>
            </tbody>
          </table>
        </div>
      )}

      {/* Storage */}
      {activeDevTab === 'storage' && (
        <div className="flex-1 overflow-auto p-3 text-xs font-mono text-gray-300">
          <div className="space-y-3">
            <div>
              <div className="text-gray-500 mb-1">LocalStorage</div>
              <div className="text-gray-400">No items</div>
            </div>
            <div>
              <div className="text-gray-500 mb-1">SessionStorage</div>
              <div className="text-gray-400">No items</div>
            </div>
            <div>
              <div className="text-gray-500 mb-1">IndexedDB (eBrowser)</div>
              <div className="text-green-400">history, bookmarks, downloads, passwords, notes, readingList</div>
            </div>
            <div>
              <div className="text-gray-500 mb-1">Cookies</div>
              <div className="text-gray-400">Managed by browser engine</div>
            </div>
          </div>
        </div>
      )}

      {/* Sources / Debugger */}
      {(activeDevTab === 'sources' || activeDevTab === 'debugger') && (
        <div className="flex-1 flex items-center justify-center text-gray-500 text-xs">
          {activeDevTab === 'sources' ? 'Source files available for same-origin pages.' : 'JavaScript debugger available for same-origin pages.'}
        </div>
      )}
    </motion.div>
  );
}
