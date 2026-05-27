import React, { useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { CheckCircle, AlertCircle, Info, AlertTriangle, X } from 'lucide-react';
import { useBrowserStore, type Toast } from '@/store/browserStore';

export default function ToastContainer() {
  const { toasts, removeToast } = useBrowserStore();

  return (
    <div className="fixed bottom-4 right-4 z-toast flex flex-col gap-2 pointer-events-none">
      <AnimatePresence>
        {toasts.map(toast => (
          <ToastItem key={toast.id} toast={toast} onClose={() => removeToast(toast.id)} />
        ))}
      </AnimatePresence>
    </div>
  );
}

function ToastItem({ toast, onClose }: { toast: Toast; onClose: () => void }) {
  useEffect(() => {
    const timer = setTimeout(onClose, toast.duration ?? 3000);
    return () => clearTimeout(timer);
  }, [toast.id, toast.duration, onClose]);

  const config = {
    success: { icon: <CheckCircle size={15} />, bg: 'bg-green-500', text: 'text-white' },
    error: { icon: <AlertCircle size={15} />, bg: 'bg-red-500', text: 'text-white' },
    warning: { icon: <AlertTriangle size={15} />, bg: 'bg-yellow-500', text: 'text-white' },
    info: { icon: <Info size={15} />, bg: 'bg-blue-500', text: 'text-white' },
  }[toast.type];

  return (
    <motion.div
      initial={{ opacity: 0, x: 50, scale: 0.9 }}
      animate={{ opacity: 1, x: 0, scale: 1 }}
      exit={{ opacity: 0, x: 50, scale: 0.9 }}
      transition={{ duration: 0.2 }}
      className={`pointer-events-auto flex items-center gap-2.5 px-4 py-2.5 rounded-xl shadow-lg ${config.bg} ${config.text} min-w-[200px] max-w-[320px]`}
    >
      <span className="flex-shrink-0">{config.icon}</span>
      <span className="text-sm flex-1">{toast.message}</span>
      <button onClick={onClose} className="flex-shrink-0 opacity-70 hover:opacity-100 transition-opacity">
        <X size={13} />
      </button>
    </motion.div>
  );
}
