import formsPlugin from '@tailwindcss/forms'

export default {
  content: ['./src/**/*.{js,ts,ejs}'],
  theme: {
    extend: {
      colors: {},
    },
  },
  plugins: [formsPlugin],
}
