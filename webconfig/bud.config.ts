import type {Bud} from '@roots/bud';
import envParser from 'env-file-parser';
import fs from 'fs';


/**
 * bud.js configuration
 */
export default async (bud: Bud) => {

  const sdkconfig = await envParser.parse('../sdkconfig');

  await bud.sh(`node ../util/cross-defs/index.js -i ../x_defs/controls.json -c ../main/controls.h -j ./src/controls.json`);

  const controls = JSON.parse(fs.readFileSync(bud.path(`@src/controls.json`), {'encoding': 'utf8'}));

  bud.setPublicPath('/');
  bud.watch([bud.path(`@src/**/*.ts`), bud.path(`@src/**/index.css`), bud.path(`@src/**/*.ejs`)])

  bud.define(Object.fromEntries(Object.entries(sdkconfig).map(([k, v]) => [k, '"' + v.replace(/[\""]/g, '\\"') + '"'])));
  bud.define(controls);
  
  bud.html({
    title: 'Cantastic Configuration',
    meta: {
      viewport: 'width=device-width, initial-scale=1, shrink-to-fit=no',
    },
    template: bud.path(`src/index.ejs`),
    minify: !bud.isProduction,
    inject: !bud.isProduction,
    isProduction: bud.isProduction
  })

  bud.entry('app', ['index.ts', 'index.css']).html();
  bud.after(async () => { 
    await bud.when(bud.isProduction, () => bud.sh(`gzip -k -f dist/index.html`))
  });
};
