{
  'sources': [
    'menu_extension.h',
    'menu_extension.cc',
  ],
  'actions': [
    {
      'action_name': 'generate_menu_api',
      'inputs': [
        'generate_api.py',
        'menu_api.js',
      ],
      'outputs': [
        '<(SHARED_INTERMEDIATE_DIR)/menu_api.h',
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
  ]
}
