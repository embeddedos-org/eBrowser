import { beforeEach, describe, expect, it, vi } from 'vitest';
import { fireEvent, render, screen } from '@testing-library/react';
import Toolbar from './Toolbar';
import NewTabPage from './pages/NewTabPage';
import { useBrowserStore } from '@/store/browserStore';

vi.mock('@/utils/database', () => ({
  bookmarkDB: { isBookmarked: async () => false, search: async () => [] },
  historyDB: { getRecent: async () => [], search: async () => [] },
}));

const initial = useBrowserStore.getState();
const store = () => useBrowserStore.getState();
const activeUrl = () => store().getActiveTab()?.url;

beforeEach(() => {
  localStorage.clear();
  useBrowserStore.setState(initial, true);
});

function submitAddressBar(value: string) {
  render(<Toolbar />);
  const input = document.getElementById('address-bar-input') as HTMLInputElement;
  fireEvent.focus(input);
  fireEvent.change(input, { target: { value } });
  fireEvent.keyDown(input, { key: 'Enter' });
}

describe('address bar', () => {
  it('searches with the engine chosen in settings', () => {
    store().updateSettings({ searchEngine: 'duckduckgo' });
    submitAddressBar('privacy browser');
    expect(activeUrl()).toBe('https://duckduckgo.com/?q=privacy%20browser');
  });

  it('searches with a custom search URL', () => {
    store().updateSettings({ searchEngine: 'custom', customSearchUrl: 'https://search.test/?q=' });
    submitAddressBar('privacy browser');
    expect(activeUrl()).toBe('https://search.test/?q=privacy%20browser');
  });

  it('uses the engine chosen after the address bar was rendered', () => {
    store().updateSettings({ searchEngine: 'duckduckgo' });
    render(<Toolbar />);
    store().updateSettings({ searchEngine: 'brave' });
    const input = document.getElementById('address-bar-input') as HTMLInputElement;
    fireEvent.focus(input);
    fireEvent.change(input, { target: { value: 'privacy browser' } });
    fireEvent.keyDown(input, { key: 'Enter' });
    expect(activeUrl()).toBe('https://search.brave.com/search?q=privacy%20browser');
  });

  it('navigates to an address instead of searching for it', () => {
    store().updateSettings({ searchEngine: 'duckduckgo' });
    submitAddressBar('example.com');
    expect(activeUrl()).toBe('https://example.com/');
  });
});

describe('new tab search box', () => {
  it('searches with the engine chosen in settings', () => {
    store().updateSettings({ searchEngine: 'brave' });
    render(<NewTabPage />);
    const box = screen.getAllByRole('textbox')[0];
    fireEvent.change(box, { target: { value: 'privacy browser' } });
    fireEvent.submit(box.closest('form') as HTMLFormElement);
    expect(activeUrl()).toBe('https://search.brave.com/search?q=privacy%20browser');
  });
});
