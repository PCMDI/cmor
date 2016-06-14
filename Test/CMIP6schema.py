from jsonschema import validate

CMIP6schema = {
    "title": "JSON schema for the CMIP6 ESGF file",
    "$schema": "http://json-schema.org/draft-04/schema#",

    "type": "object",
    "dimension": { 
        "name" : { "type" : "string" },
    },
    "variables": {
        "type": "array",
            "items": { "type": "string " },
        "time": {
            "bounds": { "type" : "time_bnds"} 
            "units": "days since 2030-1-1" ;
            "calendar": "360_day" ;
            "axis": "T" ;
            "long_name": "time" ;
            "standard_name": "time" ;
        }
    }
}


