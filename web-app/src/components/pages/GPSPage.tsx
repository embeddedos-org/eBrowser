import React, { useState, useEffect } from 'react';
import { MapPin, Navigation, Compass, Wifi, RefreshCw, Globe, Clock, Thermometer, Wind } from 'lucide-react';
import { motion } from 'framer-motion';
import { useBrowserStore } from '@/store/browserStore';

interface WeatherData {
  temp: number;
  condition: string;
  humidity: number;
  wind: number;
  city: string;
  country: string;
}

export default function GPSPage() {
  const { gpsLocation, settings, setGpsLocation, addToast } = useBrowserStore();
  const [weather, setWeather] = useState<WeatherData | null>(null);
  const [loading, setLoading] = useState(false);
  const [locationName, setLocationName] = useState<string>('');

  const requestLocation = () => {
    if (!navigator.geolocation) {
      addToast({ type: 'error', message: 'Geolocation is not supported by your browser' });
      return;
    }
    setLoading(true);
    navigator.geolocation.getCurrentPosition(
      (pos) => {
        setGpsLocation(pos.coords);
        setLoading(false);
        addToast({ type: 'success', message: 'Location updated' });
        // Reverse geocode
        fetchLocationName(pos.coords.latitude, pos.coords.longitude);
        // Fetch weather
        fetchWeather(pos.coords.latitude, pos.coords.longitude);
      },
      (err) => {
        setLoading(false);
        addToast({ type: 'error', message: `Location error: ${err.message}` });
      },
      { enableHighAccuracy: true, timeout: 10000 }
    );
  };

  const fetchLocationName = async (lat: number, lon: number) => {
    try {
      const res = await fetch(`https://nominatim.openstreetmap.org/reverse?lat=${lat}&lon=${lon}&format=json`);
      const data = await res.json();
      const city = data.address?.city || data.address?.town || data.address?.village || '';
      const country = data.address?.country || '';
      setLocationName(`${city}${city && country ? ', ' : ''}${country}`);
    } catch {
      setLocationName('Location found');
    }
  };

  const fetchWeather = async (lat: number, lon: number) => {
    try {
      // Using Open-Meteo (free, no API key required)
      const res = await fetch(
        `https://api.open-meteo.com/v1/forecast?latitude=${lat}&longitude=${lon}&current_weather=true&hourly=relativehumidity_2m,windspeed_10m`
      );
      const data = await res.json();
      if (data.current_weather) {
        setWeather({
          temp: Math.round(data.current_weather.temperature),
          condition: getWeatherCondition(data.current_weather.weathercode),
          humidity: data.hourly?.relativehumidity_2m?.[0] ?? 0,
          wind: Math.round(data.current_weather.windspeed),
          city: locationName || 'Your Location',
          country: '',
        });
      }
    } catch {
      // Weather fetch failed silently
    }
  };

  const getWeatherCondition = (code: number): string => {
    if (code === 0) return 'Clear Sky';
    if (code <= 3) return 'Partly Cloudy';
    if (code <= 9) return 'Foggy';
    if (code <= 19) return 'Drizzle';
    if (code <= 29) return 'Rain';
    if (code <= 39) return 'Snow';
    if (code <= 49) return 'Sleet';
    if (code <= 59) return 'Freezing Rain';
    if (code <= 69) return 'Heavy Snow';
    if (code <= 79) return 'Ice Pellets';
    if (code <= 84) return 'Rain Showers';
    if (code <= 94) return 'Thunderstorm';
    return 'Severe Weather';
  };

  useEffect(() => {
    if (gpsLocation) {
      fetchLocationName(gpsLocation.latitude, gpsLocation.longitude);
      fetchWeather(gpsLocation.latitude, gpsLocation.longitude);
    }
  }, []);

  return (
    <div className="max-w-2xl mx-auto p-6">
      <div className="flex items-center gap-2 mb-6">
        <MapPin size={24} className="text-primary-500" />
        <h1 className="text-2xl font-semibold text-gray-900 dark:text-gray-100">GPS & Location</h1>
      </div>

      {/* Location status */}
      <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} className="card mb-6">
        <div className="flex items-center justify-between mb-4">
          <h2 className="text-base font-semibold text-gray-900 dark:text-gray-100">Current Location</h2>
          <button onClick={requestLocation} disabled={loading} className="btn-primary gap-2 text-sm">
            {loading ? <div className="w-3 h-3 border-2 border-white border-t-transparent rounded-full animate-spin" /> : <RefreshCw size={12} />}
            {loading ? 'Getting Location...' : 'Update Location'}
          </button>
        </div>

        {gpsLocation ? (
          <div className="space-y-3">
            {locationName && (
              <div className="flex items-center gap-2 text-lg font-medium text-gray-900 dark:text-gray-100">
                <Globe size={18} className="text-primary-500" />
                {locationName}
              </div>
            )}
            <div className="grid grid-cols-2 gap-3">
              <div className="bg-gray-50 dark:bg-gray-700 rounded-lg p-3">
                <div className="text-xs text-gray-400 mb-1">Latitude</div>
                <div className="text-sm font-mono font-medium text-gray-900 dark:text-gray-100">
                  {gpsLocation.latitude.toFixed(6)}°
                </div>
              </div>
              <div className="bg-gray-50 dark:bg-gray-700 rounded-lg p-3">
                <div className="text-xs text-gray-400 mb-1">Longitude</div>
                <div className="text-sm font-mono font-medium text-gray-900 dark:text-gray-100">
                  {gpsLocation.longitude.toFixed(6)}°
                </div>
              </div>
              <div className="bg-gray-50 dark:bg-gray-700 rounded-lg p-3">
                <div className="text-xs text-gray-400 mb-1">Accuracy</div>
                <div className="text-sm font-medium text-gray-900 dark:text-gray-100">
                  ±{Math.round(gpsLocation.accuracy)}m
                </div>
              </div>
              {gpsLocation.altitude !== null && (
                <div className="bg-gray-50 dark:bg-gray-700 rounded-lg p-3">
                  <div className="text-xs text-gray-400 mb-1">Altitude</div>
                  <div className="text-sm font-medium text-gray-900 dark:text-gray-100">
                    {Math.round(gpsLocation.altitude ?? 0)}m
                  </div>
                </div>
              )}
              {gpsLocation.speed !== null && gpsLocation.speed !== undefined && (
                <div className="bg-gray-50 dark:bg-gray-700 rounded-lg p-3">
                  <div className="text-xs text-gray-400 mb-1">Speed</div>
                  <div className="text-sm font-medium text-gray-900 dark:text-gray-100">
                    {Math.round((gpsLocation.speed ?? 0) * 3.6)} km/h
                  </div>
                </div>
              )}
              {gpsLocation.heading !== null && gpsLocation.heading !== undefined && (
                <div className="bg-gray-50 dark:bg-gray-700 rounded-lg p-3">
                  <div className="text-xs text-gray-400 mb-1 flex items-center gap-1"><Compass size={10} />Heading</div>
                  <div className="text-sm font-medium text-gray-900 dark:text-gray-100">
                    {Math.round(gpsLocation.heading ?? 0)}°
                  </div>
                </div>
              )}
            </div>

            {/* Map link */}
            <a
              href={`https://www.openstreetmap.org/?mlat=${gpsLocation.latitude}&mlon=${gpsLocation.longitude}&zoom=15`}
              target="_blank"
              rel="noopener noreferrer"
              className="btn-secondary w-full gap-2 justify-center"
            >
              <MapPin size={14} />
              View on OpenStreetMap
            </a>
          </div>
        ) : (
          <div className="text-center py-8 text-gray-400">
            <Navigation size={40} className="mx-auto mb-3 opacity-30" />
            <p className="text-sm">Location not available</p>
            <p className="text-xs mt-1">Click "Update Location" to get your current position</p>
            {!settings.enableGeolocation && (
              <p className="text-xs mt-2 text-yellow-500">⚠️ Geolocation is disabled in settings</p>
            )}
          </div>
        )}
      </motion.div>

      {/* Weather */}
      {weather && (
        <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ delay: 0.1 }} className="card">
          <h2 className="text-base font-semibold text-gray-900 dark:text-gray-100 mb-4 flex items-center gap-2">
            <Thermometer size={16} className="text-orange-500" />
            Local Weather
          </h2>
          <div className="flex items-center gap-6">
            <div className="text-5xl font-light text-gray-900 dark:text-gray-100">{weather.temp}°C</div>
            <div>
              <div className="text-base font-medium text-gray-700 dark:text-gray-300">{weather.condition}</div>
              <div className="flex items-center gap-3 mt-1 text-sm text-gray-400">
                <span className="flex items-center gap-1"><Wifi size={12} />Humidity: {weather.humidity}%</span>
                <span className="flex items-center gap-1"><Wind size={12} />Wind: {weather.wind} km/h</span>
              </div>
            </div>
          </div>
          <div className="text-xs text-gray-400 mt-3">
            Powered by Open-Meteo (open-meteo.com) — Free, no API key required
          </div>
        </motion.div>
      )}

      {/* Location-based features */}
      <div className="mt-6 card">
        <h2 className="text-base font-semibold text-gray-900 dark:text-gray-100 mb-3">Location-Based Features</h2>
        <div className="space-y-2 text-sm text-gray-600 dark:text-gray-400">
          <div className="flex items-center gap-2">
            <div className="w-2 h-2 rounded-full bg-green-500" />
            Automatic timezone detection
          </div>
          <div className="flex items-center gap-2">
            <div className="w-2 h-2 rounded-full bg-green-500" />
            Location-aware search results
          </div>
          <div className="flex items-center gap-2">
            <div className="w-2 h-2 rounded-full bg-green-500" />
            Local news and weather
          </div>
          <div className="flex items-center gap-2">
            <div className="w-2 h-2 rounded-full bg-green-500" />
            Nearby places and services
          </div>
          <div className="flex items-center gap-2">
            <div className="w-2 h-2 rounded-full bg-yellow-500" />
            GPS-based content filtering (coming soon)
          </div>
        </div>
      </div>
    </div>
  );
}
