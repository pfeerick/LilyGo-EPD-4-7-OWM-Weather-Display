#!/usr/bin/env node

const express = require('express');
const fs = require('fs');
const path = require('path');

const app = express();
const port = process.env.PORT || 3000;
const webDir = path.join(__dirname, 'web');

// Mock config values shown in the preview
const mock = {
  __SSID__:         'MyHomeWiFi',
  __PASSWORD__:     '',
  __APIKEY__:       'your_api_key_here',
  __SERVER__:       'api.openweathermap.org',
  __CITY__:         'Bath',
  __LATITUDE__:     '51.38',
  __LONGITUDE__:    '-2.36',
  __HEM_NORTH__:    'selected',
  __HEM_SOUTH__:    '',
  __UNITS_M__:      'selected',
  __UNITS_I__:      '',
  __LANG_EN__:      'selected',
  __LANG_AR__:      '',
  __LANG_CZ__:      '',
  __LANG_EL__:      '',
  __LANG_FA__:      '',
  __LANG_FR__:      '',
  __LANG_GL__:      '',
  __LANG_HU__:      '',
  __LANG_JA__:      '',
  __LANG_KR__:      '',
  __LANG_LA__:      '',
  __LANG_LT__:      '',
  __LANG_MK__:      '',
  __LANG_SK__:      '',
  __LANG_SL__:      '',
  __LANG_VI__:      '',
  __TIMEZONE__:     'GMT0BST,M3.5.0/01,M10.5.0/02',
  __NTPSERVER__:    'pool.ntp.org',
  __GMTOFFSET__:    '0',
  __DSTOFFSET__:    '3600',
  __SLEEPDURATION__:'30',
  __WAKEUPHOUR__:   '7',
  __SLEEPHOUR__:    '23',
};

app.get('/', (req, res) => {
  let html = fs.readFileSync(path.join(webDir, 'config.html'), 'utf8');
  for (const [token, value] of Object.entries(mock)) {
    html = html.replaceAll(token, value);
  }
  res.send(html);
});

// Stub — log the form data but don't actually save anything
app.post('/save', express.urlencoded({ extended: false }), (req, res) => {
  console.log('POST /save (preview mode):', req.body);
  res.send(
    "<html><body style='font-family:sans-serif;max-width:400px;margin:40px auto'>" +
    "<h2>Saved!</h2><p>Preview mode &mdash; no actual save performed.</p>" +
    "<p><a href='/'>Back</a></p></body></html>"
  );
});

const server = app.listen(port, () => {
  console.log(`Preview server running at http://localhost:${port}`);
  console.log(`Serving web/ from: ${webDir}`);
});

process.on('SIGINT', () => {
  console.log('\nShutting down...');
  server.close(() => process.exit(0));
  setTimeout(() => process.exit(1), 3000);
});
