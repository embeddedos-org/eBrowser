import React, { useState, useRef, useCallback } from 'react';
import {
  View, Text, TextInput, TouchableOpacity, StyleSheet,
  SafeAreaView, StatusBar, ActivityIndicator, BackHandler,
  Platform, Share, Vibration, Keyboard,
} from 'react-native';
import { WebView, type WebViewNavigation } from 'react-native-webview';
import { useColorScheme } from 'react-native';

interface Tab {
  id: string;
  url: string;
  title: string;
  canGoBack: boolean;
  canGoForward: boolean;
  isLoading: boolean;
}

function createTab(url = 'https://www.google.com'): Tab {
  return {
    id: `tab-${Date.now()}`,
    url,
    title: 'New Tab',
    canGoBack: false,
    canGoForward: false,
    isLoading: false,
  };
}

export default function BrowserScreen() {
  const colorScheme = useColorScheme();
  const isDark = colorScheme === 'dark';

  const [tabs, setTabs] = useState<Tab[]>([createTab()]);
  const [activeTabIndex, setActiveTabIndex] = useState(0);
  const [addressText, setAddressText] = useState('https://www.google.com');
  const [isAddressEditing, setIsAddressEditing] = useState(false);
  const [showTabs, setShowTabs] = useState(false);

  const webViewRef = useRef<WebView>(null);
  const activeTab = tabs[activeTabIndex];

  const navigate = useCallback((input: string) => {
    const isUrl = /^https?:\/\//.test(input) || /^[a-z0-9-]+\.[a-z]{2,}/.test(input);
    const url = isUrl
      ? (input.startsWith('http') ? input : `https://${input}`)
      : `https://www.google.com/search?q=${encodeURIComponent(input)}`;

    setTabs(prev => prev.map((t, i) => i === activeTabIndex ? { ...t, url, isLoading: true } : t));
    setAddressText(url);
    Keyboard.dismiss();
  }, [activeTabIndex]);

  const handleNavigationStateChange = useCallback((navState: WebViewNavigation) => {
    setTabs(prev => prev.map((t, i) => i === activeTabIndex ? {
      ...t,
      url: navState.url,
      title: navState.title || navState.url,
      canGoBack: navState.canGoBack,
      canGoForward: navState.canGoForward,
      isLoading: navState.loading,
    } : t));
    if (!isAddressEditing) {
      setAddressText(navState.url);
    }
  }, [activeTabIndex, isAddressEditing]);

  const handleShare = async () => {
    try {
      await Share.share({ url: activeTab.url, title: activeTab.title, message: activeTab.url });
    } catch (e) { /* ignore */ }
  };

  const colors = {
    bg: isDark ? '#0f0f1a' : '#f8fafc',
    toolbar: isDark ? '#1e1e2e' : '#ffffff',
    border: isDark ? '#313244' : '#e2e8f0',
    text: isDark ? '#cdd6f4' : '#1e293b',
    muted: isDark ? '#6c7086' : '#64748b',
    primary: '#6366f1',
    addressBg: isDark ? '#313244' : '#f1f5f9',
  };

  return (
    <SafeAreaView style={[styles.container, { backgroundColor: colors.bg }]}>
      <StatusBar
        barStyle={isDark ? 'light-content' : 'dark-content'}
        backgroundColor={colors.toolbar}
      />

      {/* Toolbar */}
      <View style={[styles.toolbar, { backgroundColor: colors.toolbar, borderBottomColor: colors.border }]}>
        {/* Navigation buttons */}
        <TouchableOpacity
          style={[styles.navBtn, !activeTab.canGoBack && styles.navBtnDisabled]}
          onPress={() => webViewRef.current?.goBack()}
          disabled={!activeTab.canGoBack}
        >
          <Text style={[styles.navBtnText, { color: activeTab.canGoBack ? colors.text : colors.muted }]}>‹</Text>
        </TouchableOpacity>

        <TouchableOpacity
          style={[styles.navBtn, !activeTab.canGoForward && styles.navBtnDisabled]}
          onPress={() => webViewRef.current?.goForward()}
          disabled={!activeTab.canGoForward}
        >
          <Text style={[styles.navBtnText, { color: activeTab.canGoForward ? colors.text : colors.muted }]}>›</Text>
        </TouchableOpacity>

        {/* Address bar */}
        <TouchableOpacity
          style={[styles.addressBar, { backgroundColor: colors.addressBg }]}
          onPress={() => setIsAddressEditing(true)}
          activeOpacity={0.8}
        >
          {activeTab.url.startsWith('https://') && (
            <Text style={styles.secureIcon}>🔒</Text>
          )}
          {isAddressEditing ? (
            <TextInput
              style={[styles.addressInput, { color: colors.text }]}
              value={addressText}
              onChangeText={setAddressText}
              onSubmitEditing={() => { navigate(addressText); setIsAddressEditing(false); }}
              onBlur={() => setIsAddressEditing(false)}
              autoFocus
              selectTextOnFocus
              keyboardType="url"
              autoCapitalize="none"
              autoCorrect={false}
              returnKeyType="go"
              placeholder="Search or enter URL..."
              placeholderTextColor={colors.muted}
            />
          ) : (
            <Text style={[styles.addressText, { color: colors.text }]} numberOfLines={1}>
              {activeTab.url.replace(/^https?:\/\/(www\.)?/, '')}
            </Text>
          )}
          {activeTab.isLoading && <ActivityIndicator size="small" color={colors.primary} style={{ marginLeft: 4 }} />}
        </TouchableOpacity>

        {/* Reload / Stop */}
        <TouchableOpacity
          style={styles.navBtn}
          onPress={() => activeTab.isLoading ? webViewRef.current?.stopLoading() : webViewRef.current?.reload()}
        >
          <Text style={[styles.navBtnText, { color: colors.text }]}>{activeTab.isLoading ? '✕' : '↻'}</Text>
        </TouchableOpacity>

        {/* Share */}
        <TouchableOpacity style={styles.navBtn} onPress={handleShare}>
          <Text style={[styles.navBtnText, { color: colors.text }]}>⬆</Text>
        </TouchableOpacity>

        {/* Tab count */}
        <TouchableOpacity style={styles.tabCountBtn} onPress={() => setShowTabs(!showTabs)}>
          <View style={[styles.tabCountBadge, { borderColor: colors.text }]}>
            <Text style={[styles.tabCountText, { color: colors.text }]}>{tabs.length}</Text>
          </View>
        </TouchableOpacity>
      </View>

      {/* WebView */}
      {tabs.map((tab, index) => (
        <View key={tab.id} style={[styles.webviewContainer, index !== activeTabIndex && styles.hidden]}>
          <WebView
            ref={index === activeTabIndex ? webViewRef : undefined}
            source={{ uri: tab.url }}
            onNavigationStateChange={handleNavigationStateChange}
            onLoadStart={() => setTabs(prev => prev.map((t, i) => i === index ? { ...t, isLoading: true } : t))}
            onLoadEnd={() => setTabs(prev => prev.map((t, i) => i === index ? { ...t, isLoading: false } : t))}
            javaScriptEnabled
            domStorageEnabled
            allowsInlineMediaPlayback
            mediaPlaybackRequiresUserAction={false}
            allowsFullscreenVideo
            geolocationEnabled
            sharedCookiesEnabled
            thirdPartyCookiesEnabled
            allowsBackForwardNavigationGestures
            pullToRefreshEnabled
            startInLoadingState
            renderLoading={() => (
              <View style={styles.loadingContainer}>
                <ActivityIndicator size="large" color="#6366f1" />
              </View>
            )}
            userAgent={`Mozilla/5.0 (${Platform.OS === 'ios' ? 'iPhone; CPU iPhone OS 17_0 like Mac OS X' : 'Linux; Android 14'}) AppleWebKit/605.1.15 (KHTML, like Gecko) eBrowser/2.0 Mobile Safari/604.1`}
          />
        </View>
      ))}

      {/* Bottom bar */}
      <View style={[styles.bottomBar, { backgroundColor: colors.toolbar, borderTopColor: colors.border }]}>
        <TouchableOpacity style={styles.bottomBtn} onPress={() => navigate('https://www.google.com')}>
          <Text style={styles.bottomBtnIcon}>🏠</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.bottomBtn} onPress={() => {
          const newTab = createTab();
          setTabs(prev => [...prev, newTab]);
          setActiveTabIndex(tabs.length);
          setAddressText(newTab.url);
        }}>
          <Text style={styles.bottomBtnIcon}>＋</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.bottomBtn} onPress={() => navigate('ebrowser://bookmarks')}>
          <Text style={styles.bottomBtnIcon}>⭐</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.bottomBtn} onPress={() => navigate('ebrowser://history')}>
          <Text style={styles.bottomBtnIcon}>🕐</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.bottomBtn} onPress={() => navigate('ebrowser://settings')}>
          <Text style={styles.bottomBtnIcon}>⚙️</Text>
        </TouchableOpacity>
      </View>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1 },
  toolbar: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: 8,
    paddingVertical: 8,
    borderBottomWidth: 1,
    gap: 4,
  },
  navBtn: { padding: 8, borderRadius: 8, minWidth: 36, alignItems: 'center' },
  navBtnDisabled: { opacity: 0.4 },
  navBtnText: { fontSize: 22, fontWeight: '300' },
  addressBar: {
    flex: 1,
    flexDirection: 'row',
    alignItems: 'center',
    borderRadius: 12,
    paddingHorizontal: 12,
    paddingVertical: 8,
    gap: 6,
  },
  secureIcon: { fontSize: 12 },
  addressInput: { flex: 1, fontSize: 14, padding: 0 },
  addressText: { flex: 1, fontSize: 14 },
  tabCountBtn: { padding: 8 },
  tabCountBadge: {
    width: 26, height: 26,
    borderRadius: 6,
    borderWidth: 2,
    alignItems: 'center',
    justifyContent: 'center',
  },
  tabCountText: { fontSize: 12, fontWeight: '700' },
  webviewContainer: { flex: 1 },
  hidden: { position: 'absolute', width: 0, height: 0, opacity: 0 },
  loadingContainer: { flex: 1, alignItems: 'center', justifyContent: 'center' },
  bottomBar: {
    flexDirection: 'row',
    borderTopWidth: 1,
    paddingBottom: Platform.OS === 'ios' ? 16 : 8,
  },
  bottomBtn: { flex: 1, alignItems: 'center', justifyContent: 'center', paddingVertical: 10 },
  bottomBtnIcon: { fontSize: 20 },
});
