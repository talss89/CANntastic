import type {Bud} from '@roots/bud';

/**
 * bud.js configuration
 */

export default async (bud: Bud) => {
  bud.watch([bud.path(`@src/**/*.ts`), bud.path(`@src/**/index.css`), bud.path(`@src/**/*.ejs`)])
  bud.entry('app', ['index.ts', 'index.css', 'controls.xd.json'])
  bud.setProxyUrl('http://' + (process.env.DEVICE_IP ?? '192.168.4.1'))

  bud.embedded.set('body', bud.path('@src/app.html'));
  bud.embedded.crossDefine({
    manifest: bud.path('@src/controls.xd.json'),
    langs: ['c']
  })
  bud.embedded.set('compress', 'gzip')
};
