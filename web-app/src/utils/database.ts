import Dexie, { type Table } from 'dexie';

export interface HistoryEntry {
  id?: number;
  url: string;
  title: string;
  favicon?: string;
  visitedAt: Date;
  visitCount: number;
  lastVisitDuration?: number; // seconds
  deviceType?: string;
}

export interface Bookmark {
  id?: number;
  url: string;
  title: string;
  favicon?: string;
  folderId?: number;
  createdAt: Date;
  tags?: string[];
  description?: string;
  pinned?: boolean;
}

export interface BookmarkFolder {
  id?: number;
  name: string;
  parentId?: number;
  createdAt: Date;
  icon?: string;
  color?: string;
}

export interface DownloadItem {
  id?: number;
  url: string;
  filename: string;
  mimeType?: string;
  size?: number;
  downloadedBytes?: number;
  status: 'pending' | 'downloading' | 'completed' | 'failed' | 'cancelled' | 'paused';
  startedAt: Date;
  completedAt?: Date;
  localPath?: string;
  error?: string;
}

export interface CookieRecord {
  id?: number;
  domain: string;
  name: string;
  value: string;
  path: string;
  expires?: Date;
  secure: boolean;
  httpOnly: boolean;
  sameSite: 'Strict' | 'Lax' | 'None';
  createdAt: Date;
}

export interface PasswordEntry {
  id?: number;
  url: string;
  username: string;
  encryptedPassword: string;
  createdAt: Date;
  updatedAt: Date;
  lastUsed?: Date;
  notes?: string;
}

export interface TabSession {
  id?: number;
  name: string;
  tabs: Array<{ url: string; title: string; favicon?: string }>;
  createdAt: Date;
  isAutoSave?: boolean;
}

export interface ExtensionRecord {
  id?: string;
  name: string;
  version: string;
  description: string;
  enabled: boolean;
  permissions: string[];
  iconUrl?: string;
  installedAt: Date;
  manifest: Record<string, unknown>;
  storage?: Record<string, unknown>;
}

export interface SearchSuggestion {
  id?: number;
  query: string;
  usedAt: Date;
  useCount: number;
}

export interface ReadingListItem {
  id?: number;
  url: string;
  title: string;
  excerpt?: string;
  favicon?: string;
  addedAt: Date;
  isRead: boolean;
  readAt?: Date;
  estimatedReadTime?: number; // minutes
}

export interface Note {
  id?: number;
  title: string;
  content: string;
  url?: string;
  createdAt: Date;
  updatedAt: Date;
  tags?: string[];
}

export interface SitePermission {
  id?: number;
  origin: string;
  permission: 'geolocation' | 'notifications' | 'camera' | 'microphone' | 'clipboard' | 'storage';
  state: 'granted' | 'denied' | 'prompt';
  updatedAt: Date;
}

class EBrowserDatabase extends Dexie {
  history!: Table<HistoryEntry>;
  bookmarks!: Table<Bookmark>;
  bookmarkFolders!: Table<BookmarkFolder>;
  downloads!: Table<DownloadItem>;
  cookies!: Table<CookieRecord>;
  passwords!: Table<PasswordEntry>;
  tabSessions!: Table<TabSession>;
  extensions!: Table<ExtensionRecord>;
  searchSuggestions!: Table<SearchSuggestion>;
  readingList!: Table<ReadingListItem>;
  notes!: Table<Note>;
  sitePermissions!: Table<SitePermission>;

  constructor() {
    super('eBrowserDB');
    this.version(1).stores({
      history: '++id, url, visitedAt, visitCount',
      bookmarks: '++id, url, folderId, createdAt, pinned',
      bookmarkFolders: '++id, parentId, name',
      downloads: '++id, status, startedAt',
      cookies: '++id, domain, name',
      passwords: '++id, url, username',
      tabSessions: '++id, name, createdAt',
      extensions: 'id, name, enabled',
      searchSuggestions: '++id, query, usedAt',
      readingList: '++id, url, addedAt, isRead',
      notes: '++id, title, createdAt',
      sitePermissions: '++id, origin, permission',
    });
  }
}

export const db = new EBrowserDatabase();

// History helpers
export const historyDB = {
  async add(url: string, title: string, favicon?: string) {
    const existing = await db.history.where('url').equals(url).first();
    if (existing?.id) {
      await db.history.update(existing.id, {
        visitedAt: new Date(),
        visitCount: (existing.visitCount || 1) + 1,
        title,
        favicon,
      });
    } else {
      await db.history.add({ url, title, favicon, visitedAt: new Date(), visitCount: 1 });
    }
  },
  async getRecent(limit = 100) {
    return db.history.orderBy('visitedAt').reverse().limit(limit).toArray();
  },
  async search(query: string) {
    const q = query.toLowerCase();
    return db.history
      .filter(h => h.url.toLowerCase().includes(q) || h.title.toLowerCase().includes(q))
      .limit(20)
      .toArray();
  },
  async clear() {
    return db.history.clear();
  },
  async deleteEntry(id: number) {
    return db.history.delete(id);
  },
};

// Bookmark helpers
export const bookmarkDB = {
  async add(bookmark: Omit<Bookmark, 'id' | 'createdAt'>) {
    return db.bookmarks.add({ ...bookmark, createdAt: new Date() });
  },
  async getAll() {
    return db.bookmarks.orderBy('createdAt').reverse().toArray();
  },
  async getByFolder(folderId?: number) {
    if (folderId === undefined) {
      return db.bookmarks.filter(b => !b.folderId).toArray();
    }
    return db.bookmarks.where('folderId').equals(folderId).toArray();
  },
  async search(query: string) {
    const q = query.toLowerCase();
    return db.bookmarks
      .filter(b => b.url.toLowerCase().includes(q) || b.title.toLowerCase().includes(q))
      .toArray();
  },
  async delete(id: number) {
    return db.bookmarks.delete(id);
  },
  async update(id: number, changes: Partial<Bookmark>) {
    return db.bookmarks.update(id, changes);
  },
  async isBookmarked(url: string) {
    const count = await db.bookmarks.where('url').equals(url).count();
    return count > 0;
  },
  async getByUrl(url: string) {
    return db.bookmarks.where('url').equals(url).first();
  },
};

// Download helpers
export const downloadDB = {
  async add(item: Omit<DownloadItem, 'id'>) {
    return db.downloads.add(item);
  },
  async getAll() {
    return db.downloads.orderBy('startedAt').reverse().toArray();
  },
  async update(id: number, changes: Partial<DownloadItem>) {
    return db.downloads.update(id, changes);
  },
  async delete(id: number) {
    return db.downloads.delete(id);
  },
  async clearCompleted() {
    return db.downloads.where('status').anyOf(['completed', 'failed', 'cancelled']).delete();
  },
};

// Reading list helpers
export const readingListDB = {
  async add(item: Omit<ReadingListItem, 'id' | 'addedAt' | 'isRead'>) {
    return db.readingList.add({ ...item, addedAt: new Date(), isRead: false });
  },
  async getAll() {
    return db.readingList.orderBy('addedAt').reverse().toArray();
  },
  async markRead(id: number) {
    return db.readingList.update(id, { isRead: true, readAt: new Date() });
  },
  async delete(id: number) {
    return db.readingList.delete(id);
  },
};

// Password helpers
export const passwordDB = {
  async save(entry: Omit<PasswordEntry, 'id' | 'createdAt' | 'updatedAt'>) {
    const existing = await db.passwords.where('url').equals(entry.url).and(p => p.username === entry.username).first();
    if (existing?.id) {
      return db.passwords.update(existing.id, { ...entry, updatedAt: new Date() });
    }
    return db.passwords.add({ ...entry, createdAt: new Date(), updatedAt: new Date() });
  },
  async getForUrl(url: string) {
    return db.passwords.where('url').equals(url).toArray();
  },
  async getAll() {
    return db.passwords.toArray();
  },
  async delete(id: number) {
    return db.passwords.delete(id);
  },
};
