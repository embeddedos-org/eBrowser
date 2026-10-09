import { beforeEach, describe, expect, it, vi } from 'vitest';
import type { ReactNode } from 'react';
import { fireEvent, render, screen, waitFor } from '@testing-library/react';

type MockFn = { mockResolvedValue: (value: unknown) => void };

vi.mock('@/utils/database', () => ({
  bookmarkDB: {
    getAll: vi.fn(),
    search: vi.fn(),
    update: vi.fn(),
    delete: vi.fn(),
  },
}));

vi.mock('react-i18next', () => ({
  useTranslation: () => ({ t: (k: string) => k }),
}));

vi.mock('@/store/browserStore', () => ({
  useBrowserStore: () => ({
    addToast: vi.fn(),
    getActiveTab: () => null,
  }),
}));

vi.mock('framer-motion', () => ({
  motion: {
    div: ({ children, ...props }: { children?: ReactNode; [key: string]: unknown }) => (
      <div {...props}>{children}</div>
    ),
  },
}));

import { bookmarkDB } from '@/utils/database';
import BookmarksPage from './BookmarksPage';

const bookmarks = [
  { id: 1, url: 'https://example.com', title: 'Example', createdAt: new Date() },
];

beforeEach(() => {
  vi.clearAllMocks();
  (bookmarkDB.getAll as unknown as MockFn).mockResolvedValue(bookmarks);
  (bookmarkDB.update as unknown as MockFn).mockResolvedValue(1);
});

describe('BookmarksPage inline edit (#45)', () => {
  it('opens an inline edit form prefilled with the bookmark title and URL', async () => {
    render(<BookmarksPage />);
    await waitFor(() => expect(screen.getByText('Example')).toBeInTheDocument());

    fireEvent.click(screen.getByTitle('bookmarks.editBookmark'));

    expect(screen.getByLabelText('bookmarks.bookmarkName')).toHaveValue('Example');
    expect(screen.getByLabelText('bookmarks.bookmarkUrl')).toHaveValue('https://example.com');
  });

  it('saves the edited title and URL through bookmarkDB.update', async () => {
    render(<BookmarksPage />);
    await waitFor(() => expect(screen.getByText('Example')).toBeInTheDocument());

    fireEvent.click(screen.getByTitle('bookmarks.editBookmark'));
    fireEvent.change(screen.getByLabelText('bookmarks.bookmarkName'), {
      target: { value: 'Renamed' },
    });
    fireEvent.click(screen.getByText('common.save'));

    await waitFor(() =>
      expect(bookmarkDB.update).toHaveBeenCalledWith(1, {
        title: 'Renamed',
        url: 'https://example.com',
      })
    );
    expect(screen.getByText('Renamed')).toBeInTheDocument();
  });

  it('cancel discards the edit without touching the database', async () => {
    render(<BookmarksPage />);
    await waitFor(() => expect(screen.getByText('Example')).toBeInTheDocument());

    fireEvent.click(screen.getByTitle('bookmarks.editBookmark'));
    fireEvent.change(screen.getByLabelText('bookmarks.bookmarkName'), {
      target: { value: 'Renamed' },
    });
    fireEvent.click(screen.getByText('common.cancel'));

    expect(bookmarkDB.update).not.toHaveBeenCalled();
    expect(screen.getByText('Example')).toBeInTheDocument();
  });

  it('does not save when the title is blank', async () => {
    render(<BookmarksPage />);
    await waitFor(() => expect(screen.getByText('Example')).toBeInTheDocument());

    fireEvent.click(screen.getByTitle('bookmarks.editBookmark'));
    fireEvent.change(screen.getByLabelText('bookmarks.bookmarkName'), {
      target: { value: '   ' },
    });
    fireEvent.click(screen.getByText('common.save'));

    expect(bookmarkDB.update).not.toHaveBeenCalled();
  });
});
