import controls from './controls.xd.json'
import sdkconfig from '../../sdkconfig'


import Alpine from 'alpinejs'
 
//@ts-ignore
window.Alpine = Alpine

Alpine.data('config', () => ({
  data: {},
  async init() {
    let response = await this.fetchConfig();
    this.data = await response.json();
  },
  async fetchConfig() {
    return fetch('/config')
  },
  scheme: (typeof(sdkconfig.CONFIG_CT_SCHEME_BASIC) !== 'undefined' ? 'basic' : null) ??
          (typeof(sdkconfig.CONFIG_CT_SCHEME_WEBCONFIG) !== 'undefined' ? 'webconfig' : null) ?? 
          'unknown'
}));

Alpine.start();

// document.querySelector(`#root`).innerHTML = `
//   <div class="app">
//     <div class="header">
//       <div class="logo">Hello!</div> 
//       Edit <code>src/components/App.js</code> and save to reload
//     </div>
//   </div>
// `

/**
 * Hot reload (can be removed by React and Vue users)
 */
if (import.meta.webpackHot) {
  import.meta.webpackHot.accept(console.error)
}
