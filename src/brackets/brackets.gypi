{
  'sources': [
    'brackets_extension.h',
    'brackets_extension.cc',
    'brackets_platform.h',
    'brackets_platform_gtk.cc',
  ],
  'actions': [
    {
      'action_name': 'generate_brackets_api',
      'inputs': [
        'generate_api.py',
        'brackets_api.js',
      ],
      'outputs': [
        '<(SHARED_INTERMEDIATE_DIR)/brackets_api.h'
      ],
      'action': [
        'python',
        '<@(_inputs)',
        '<@(_outputs)',
      ],
    },
  ],
  'include_dirs': [
    '<(SHARED_INTERMEDIATE_DIR)',
  ],
}
