import { useEffect } from 'react';
import { useBrowserStore } from '@/store/browserStore';

export function useGPS() {
  const { settings, setGpsLocation } = useBrowserStore();

  useEffect(() => {
    if (!settings.enableGeolocation) return;
    if (!('geolocation' in navigator)) return;

    let watchId: number;

    const success = (position: GeolocationPosition) => {
      setGpsLocation(position.coords);
    };

    const error = (err: GeolocationPositionError) => {
      console.warn('GPS error:', err.message);
    };

    const options: PositionOptions = {
      enableHighAccuracy: true,
      timeout: 10000,
      maximumAge: 30000,
    };

    // Get initial position
    navigator.geolocation.getCurrentPosition(success, error, options);

    // Watch for updates
    watchId = navigator.geolocation.watchPosition(success, error, options);

    return () => {
      if (watchId) navigator.geolocation.clearWatch(watchId);
    };
  }, [settings.enableGeolocation, setGpsLocation]);
}
