import React, { useState, useEffect } from 'react';
import { FileText, Plus, Trash2, Edit2, Save, X } from 'lucide-react';
import { motion, AnimatePresence } from 'framer-motion';
import { db, type Note } from '@/utils/database';
import { format } from 'date-fns';

export default function NotesPage() {
  const [notes, setNotes] = useState<Note[]>([]);
  const [editing, setEditing] = useState<Note | null>(null);
  const [isNew, setIsNew] = useState(false);

  useEffect(() => { db.notes.orderBy('updatedAt').reverse().toArray().then(setNotes); }, []);

  const handleSave = async () => {
    if (!editing) return;
    if (isNew) {
      const id = await db.notes.add({ ...editing, createdAt: new Date(), updatedAt: new Date() });
      setNotes(prev => [{ ...editing, id: id as number }, ...prev]);
    } else {
      await db.notes.update(editing.id!, { ...editing, updatedAt: new Date() });
      setNotes(prev => prev.map(n => n.id === editing.id ? { ...editing, updatedAt: new Date() } : n));
    }
    setEditing(null);
    setIsNew(false);
  };

  const handleDelete = async (id: number) => {
    await db.notes.delete(id);
    setNotes(prev => prev.filter(n => n.id !== id));
  };

  const handleNew = () => {
    setEditing({ title: '', content: '', createdAt: new Date(), updatedAt: new Date() });
    setIsNew(true);
  };

  return (
    <div className="flex h-full">
      {/* Notes list */}
      <div className="w-64 flex-shrink-0 border-r border-gray-200 dark:border-gray-700 overflow-y-auto">
        <div className="p-3 border-b border-gray-200 dark:border-gray-700 flex items-center justify-between">
          <h2 className="text-sm font-semibold text-gray-900 dark:text-gray-100 flex items-center gap-1.5"><FileText size={14} />Notes</h2>
          <button onClick={handleNew} className="icon-btn w-6 h-6 text-primary-500"><Plus size={14} /></button>
        </div>
        <div className="p-2 space-y-1">
          {notes.map(note => (
            <button key={note.id} onClick={() => { setEditing(note); setIsNew(false); }}
              className={`w-full text-left p-2.5 rounded-lg transition-colors ${editing?.id === note.id ? 'bg-primary-50 dark:bg-primary-900/20' : 'hover:bg-gray-100 dark:hover:bg-gray-800'}`}
            >
              <div className="text-sm font-medium text-gray-900 dark:text-gray-100 truncate">{note.title || 'Untitled'}</div>
              <div className="text-xs text-gray-400 truncate mt-0.5">{note.content.slice(0, 50)}</div>
              <div className="text-xs text-gray-300 dark:text-gray-600 mt-0.5">{format(new Date(note.updatedAt), 'MMM d')}</div>
            </button>
          ))}
          {notes.length === 0 && (
            <div className="text-center py-8 text-gray-400 text-sm">
              <FileText size={24} className="mx-auto mb-2 opacity-30" />
              No notes yet
            </div>
          )}
        </div>
      </div>

      {/* Editor */}
      <div className="flex-1 flex flex-col overflow-hidden">
        {editing ? (
          <>
            <div className="flex items-center gap-2 p-3 border-b border-gray-200 dark:border-gray-700">
              <input
                type="text"
                value={editing.title}
                onChange={(e) => setEditing({ ...editing, title: e.target.value })}
                placeholder="Note title..."
                className="flex-1 bg-transparent outline-none text-base font-semibold text-gray-900 dark:text-gray-100"
              />
              <button onClick={handleSave} className="btn-primary gap-1.5 text-sm py-1"><Save size={12} />Save</button>
              {!isNew && <button onClick={() => editing.id && handleDelete(editing.id)} className="icon-btn w-7 h-7 text-red-400"><Trash2 size={14} /></button>}
              <button onClick={() => { setEditing(null); setIsNew(false); }} className="icon-btn w-7 h-7"><X size={14} /></button>
            </div>
            <textarea
              value={editing.content}
              onChange={(e) => setEditing({ ...editing, content: e.target.value })}
              placeholder="Start writing..."
              className="flex-1 p-4 bg-transparent outline-none text-sm text-gray-900 dark:text-gray-100 resize-none font-mono"
            />
          </>
        ) : (
          <div className="flex-1 flex items-center justify-center text-gray-400">
            <div className="text-center">
              <FileText size={40} className="mx-auto mb-3 opacity-30" />
              <p>Select a note or create a new one</p>
              <button onClick={handleNew} className="btn-primary mt-4 gap-2"><Plus size={14} />New Note</button>
            </div>
          </div>
        )}
      </div>
    </div>
  );
}
