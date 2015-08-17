{
  "targets": [
    {
      "target_name": "addon",
      "sources": [ "hello.cc" ],

      'link_settings': {
          'libraries': [
              '-ltelldus-core'
          ]
      }
    }
  ]
}
