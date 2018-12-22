project('waster', 'c')
gtkdep = dependency('gtk+-4.0')
soupdep = dependency('libsoup-2.4')
jsondep = dependency('json-glib-1.0')
gstdep  = dependency('gstreamer-video-1.0')


gnome = import('gnome')

resources = gnome.compile_resources(
  'resources',
  'data/resources.xml',
  source_dir: '.'
)

gnome.compile_schemas()
install_data(
  'org.baedert.waster.gschema.xml',
  install_dir: 'share/glib-2.0/schemas'
)

sources = [
  'src/waster.c',
  'src/waster-main-window.c',
  'src/waster-image-loader.c',
  'src/waster-initial-state.c',
  'src/waster-album-view.c',
  'src/waster-media.c',
  'src/waster-impostor.c',
  'src/waster-remote-media-file.c',
  'src/waster-placeholder.c',
  'src/imgur-proxy.c',
  'src/imgur-proxy-call.c',
  'src/imgur-gallery.c',
  'src/main.c',
  resources,

  # Copied from Corebird
  'src/CbAnimation.c',

  # Librest
  'src/rest/rest-param.c',
  'src/rest/rest-params.c',
  'src/rest/rest-proxy.c',
  'src/rest/rest-proxy-call.c',
  'src/rest/rest-main.c',
  'src/rest/oauth-proxy.c',
  'src/rest/oauth-proxy-call.c',
  'src/rest/oauth2-proxy.c',
  'src/rest/oauth2-proxy-call.c',
  'src/rest/sha1.c',
]

cdata = configuration_data()
configure_file(
  input : 'src/config.h.meson',
  output: 'config.h',
  configuration: cdata
)

executable(
  'waster',
  sources: sources,
  dependencies: [gtkdep,
                 soupdep,
                 jsondep,
                 gstdep],
  include_directories: [
    include_directories('src/'),
    include_directories('src/rest/')
  ]
)
