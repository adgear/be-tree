# File format
---
1. betree_defs
    a. Format: `<variable name>|<type>|<allow undefined>|<min value>|<max value>`
    b. Min value and max value is optional

2. betree_constants
    a. Format: `expression id,campaign_group_id,campaign_id,advertiser_id,flight_id`

3. betree_exprs
    a. List of boolean expression. Count of betree_exprs and betree_constants should be same
    b. Format: `<boolean expression>`

4. betree_events
    a. List of query value in json format, where key is variable definition and value to match against boolean expression
