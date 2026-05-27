import { useEffect } from 'react';
import { useBrowserStore } from '@/store/browserStore';

export function useNetworkStatus() {
  const { setNetworkStatus, addToast } = useBrowserStore();

  useEffect(() => {
    const handleOnline = () => {
      setNetworkStatus('online');
      addToast({ type: 'success', message: 'Back online', duration: 2000 });
    };
    const handleOffline = () => {
      setNetworkStatus('offline');
      addToast({ type: 'warning', message: 'You are offline', duration: 5000 });
    };

    window.addEventListener('online', handleOnline);
    window.addEventListener('offline', handleOffline);

    // Check connection type if available
    const connection = (navigator as Navigator & { connection?: { effectiveType: string; addEventListener: (e: string, h: () => void) => void; removeEventListener: (e: string, h: () => void) => void } }).connection;
    if (connection) {
      const handleConnectionChange = () => {
        const type = connection.effectiveType;
        if (type === '2g' || type === 'slow-2g') {
          setNetworkStatus('slow');
        } else {
          setNetworkStatus(navigator.onLine ? 'online' : 'offline');
        }
      };
      connection.addEventListener('change', handleConnectionChange);
      handleConnectionChange();
      return () => {
        window.removeEventListener('online', handleOnline);
        window.removeEventListener('offline', handleOffline);
        connection.removeEventListener('change', handleConnectionChange);
      };
    }

    return () => {
      window.removeEventListener('online', handleOnline);
      window.removeEventListener('offline', handleOffline);
    };
  }, [setNetworkStatus, addToast]);
}
