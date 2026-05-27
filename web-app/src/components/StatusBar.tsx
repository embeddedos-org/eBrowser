import React from 'react';
import { Shield, Wifi, WifiOff, MapPin, Zap, Lock } from 'lucide-react';
import { useBrowserStore } from '@/store/browserStore';
import { getSecurityLevel } from '@/utils/url';

export default function StatusBar() {
  const { getActiveTab, networkStatus, gpsLocation, settings } = useBrowserStore();
  const activeTab = getActiveTab();
  const securityLevel = activeTab ? getSecurityLevel(activeTab.url) : 'local';

  return (
    <div className="flex items-center justify-between px-3 h-5 bg-gray-100 dark:bg-gray-900 border-t border-gray-200 dark:border-gray-700 text-[10px] text-gray-400 dark:text-gray-500">
      {/* Left: URL on hover */}
      <div className="flex items-center gap-2 flex-1 min-w-0">
        {activeTab?.isLoading && (
          <span className="text-primary-500">Loading...</span>
        )}
        {!activeTab?.isLoading && activeTab?.url && (
          <span className="truncate">{activeTab.url}</span>
        )}
      </div>

      {/* Right: indicators */}
      <div className="flex items-center gap-2 flex-shrink-0">
        {/* Security */}
        {securityLevel === 'secure' && (
          <span className="flex items-center gap-0.5 text-green-500">
            <Lock size={9} />
            Secure
          </span>
        )}
        {securityLevel === 'warning' && (
          <span className="flex items-center gap-0.5 text-yellow-500">
            <Shield size={9} />
            Not Secure
          </span>
        )}

        {/* Privacy */}
        {settings.blockTrackers && (
          <span className="flex items-center gap-0.5 text-purple-400">
            <Shield size={9} />
            Protected
          </span>
        )}

        {/* GPS */}
        {gpsLocation && (
          <span className="flex items-center gap-0.5 text-primary-400">
            <MapPin size={9} />
            GPS
          </span>
        )}

        {/* Extensions */}
        {settings.enableExtensions && (
          <span className="flex items-center gap-0.5 text-yellow-400">
            <Zap size={9} />
            Ext
          </span>
        )}

        {/* Network */}
        {networkStatus === 'offline' ? (
          <span className="flex items-center gap-0.5 text-red-400">
            <WifiOff size={9} />
            Offline
          </span>
        ) : (
          <span className="flex items-center gap-0.5 text-green-400">
            <Wifi size={9} />
            Online
          </span>
        )}

        {/* Zoom */}
        {activeTab && activeTab.zoom !== 100 && (
          <span>{activeTab.zoom}%</span>
        )}
      </div>
    </div>
  );
}
