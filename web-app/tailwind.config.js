/** @type {import('tailwindcss').Config} */
export default {
  content: ['./index.html', './src/**/*.{js,ts,jsx,tsx}'],
  darkMode: 'class',
  theme: {
    extend: {
      colors: {
        primary: {
          50: '#e8f0fe',
          100: '#d2e3fc',
          200: '#aecbfa',
          300: '#7baaf7',
          400: '#4285f4',
          500: '#1a73e8',
          600: '#1557b0',
          700: '#0d47a1',
          800: '#0a3880',
          900: '#072a60',
        },
        surface: {
          DEFAULT: '#ffffff',
          dark: '#202124',
          elevated: '#f8f9fa',
          'elevated-dark': '#292a2d',
        },
        toolbar: {
          DEFAULT: '#f1f3f4',
          dark: '#35363a',
        },
        tab: {
          active: '#ffffff',
          inactive: '#dee1e6',
          'active-dark': '#3c4043',
          'inactive-dark': '#2d2e30',
        }
      },
      fontFamily: {
        sans: ['Inter', 'system-ui', '-apple-system', 'BlinkMacSystemFont', 'Segoe UI', 'sans-serif'],
        mono: ['JetBrains Mono', 'Fira Code', 'Consolas', 'monospace'],
      },
      animation: {
        'fade-in': 'fadeIn 0.15s ease-out',
        'slide-up': 'slideUp 0.2s ease-out',
        'slide-down': 'slideDown 0.2s ease-out',
        'scale-in': 'scaleIn 0.15s ease-out',
        'spin-slow': 'spin 2s linear infinite',
        'pulse-dot': 'pulseDot 1.5s ease-in-out infinite',
      },
      keyframes: {
        fadeIn: { '0%': { opacity: '0' }, '100%': { opacity: '1' } },
        slideUp: { '0%': { transform: 'translateY(8px)', opacity: '0' }, '100%': { transform: 'translateY(0)', opacity: '1' } },
        slideDown: { '0%': { transform: 'translateY(-8px)', opacity: '0' }, '100%': { transform: 'translateY(0)', opacity: '1' } },
        scaleIn: { '0%': { transform: 'scale(0.95)', opacity: '0' }, '100%': { transform: 'scale(1)', opacity: '1' } },
        pulseDot: { '0%, 100%': { opacity: '1' }, '50%': { opacity: '0.4' } },
      },
      boxShadow: {
        'toolbar': '0 1px 3px rgba(0,0,0,0.12)',
        'panel': '0 4px 20px rgba(0,0,0,0.15)',
        'dropdown': '0 8px 24px rgba(0,0,0,0.12)',
        'tab': '0 -1px 0 rgba(0,0,0,0.08)',
      },
      borderRadius: {
        'tab': '8px 8px 0 0',
      },
      zIndex: {
        'toolbar': '100',
        'dropdown': '200',
        'modal': '300',
        'toast': '400',
        'tooltip': '500',
      }
    }
  },
  plugins: [
    require('@tailwindcss/typography'),
  ]
};
