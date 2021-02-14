# Time Periods

Each DHO can have its own time periods or use an existing one. The format is below. There's a document for each time period. 

The start period has an edge from the root_node named start. 


The graph spec is:
```
    dho/root_node   ---->   start       ---->   period

    period          ---->   next        ---->   period
```

``` json
{
   "content_groups": [[{
            "label": "content_group_label",
            "value": [
              "string",
              "details"
            ]
          },{
            "label": "start_time",
            "value": [
              "time_point",
              "2021-05-11T19:01:00.000"
            ]
          },{
            "label": "label",
            "value": [
              "string",
              "New Moon"
            ]
          }
        ],[{
            "label": "content_group_label",
            "value": [
              "string",
              "system"
            ]
          },{
            "label": "type",
            "value": [
              "name",
              "period"
            ]
          },{
            "label": "node_label",
            "value": [
              "string",
              "New Moon"
            ]
          }
        ]
      ]
}
```