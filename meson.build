project('waster', 'c')
gtkdep = dependency('gtk+-3.0')
restdep = dependency('rest-0.7')
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
  'waster.c',
  'waster-main-window.c',
  'waster-image-loader.c',
  'waster-initial-state.c',
  'waster-album-view.c',
  'waster-media.c',
  'waster-impostor.c',
  'imgur-proxy.c',
  'imgur-proxy-call.c',
  'main.c',
  resources
]

executable(
  'waster',
  sources: sources,
  dependencies: [gtkdep,
                 restdep,
                 soupdep,
                 jsondep,
                 gstdep]
)
