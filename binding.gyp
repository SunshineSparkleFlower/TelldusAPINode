{
  "targets": [
    {
      "target_name": "addon",
      "sources": [ "telldus-core_node.cc" ],

      'link_settings': {
          'libraries': [
              '-ltelldus-core'
          ]
      }
    }
  ]
}
