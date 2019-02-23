/*
         1         2         3         4         5         6         7
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*/

function StoneCheckSerializedValueType(value: any, typeStr: string)
{
  StoneCheckSerializedValueTypeGeneric(value);

  if (value['type'] != typeStr)
  {
    throw new Error(
      `Cannot deserialize type ${value['type']} into ${typeStr}`);
  }
}

function isString(val: any) :boolean
{
  return ((typeof val === 'string') || (val instanceof String));
}

function StoneCheckSerializedValueTypeGeneric(value: any)
{
  // console.//log("+-------------------------------------------------+");
  // console.//log("|            StoneCheckSerializedValueTypeGeneric |");
  // console.//log("+-------------------------------------------------+");
  // console.//log("value = ");
  // console.//log(value);
  if ( (!('type' in value)) || (!isString(value.type)) )
  {
    throw new Error(
      "Cannot deserialize value ('type' key invalid)");
  }
}

// end of generic methods
{% for enum in enums%}
export enum {{enum['name']}} {
  {% for key in enum['fields']%}{{key}},
  {%endfor%}
};
{%endfor%}

{% for struct in structs%}  export class {{struct['name']}} {
{% for key in struct['fields']%}    {{key}}:{{CanonToTs(struct['fields'][key])}};
{% endfor %}
  constructor() {
{% for key in struct['fields']%}{% if NeedsTsConstruction(enums,CanonToTs(struct['fields'][key])) %}      this.{{key}} = new {{CanonToTs(struct['fields'][key])}}();
{% endif %}{% endfor %}    }

  public StoneSerialize(): string {
    let container: object = {};
    container['type'] = '{{rootName}}.{{struct['name']}}';
    container['value'] = this;
    return JSON.stringify(container);
  }

  public static StoneDeserialize(valueStr: string) : {{struct['name']}}
  {
    let value: any = JSON.parse(valueStr);
    StoneCheckSerializedValueType(value, '{{rootName}}.{{struct['name']}}');
    let result: {{struct['name']}} = value['value'] as {{struct['name']}};
    return result;
  }
}

{% endfor %}

export interface IHandler {
  {% for struct in structs%}    Handle{{struct['name']}}(value:  {{struct['name']}}): boolean;
  {% endfor %}
};

/** Service function for StoneDispatchToHandler */
export function StoneDispatchJsonToHandler(
  jsonValue: any, handler: IHandler): boolean
{
  StoneCheckSerializedValueTypeGeneric(jsonValue);
  let type: string = jsonValue["type"];
  if (type == "")
  {
    // this should never ever happen
    throw new Error("Caught empty type while dispatching");
  }
{% for struct in structs%}    else if (type == "{{rootName}}.{{struct['name']}}")
  {
    let value = jsonValue["value"] as {{struct['name']}};
    return handler.Handle{{struct['name']}}(value);
  }
{% endfor %}
  else
  {
    return false;
  }
}

/** Takes a serialized type and passes this to the handler */
export function StoneDispatchToHandler(
  strValue: string, handler: IHandler): boolean
{
  // console.//log("+------------------------------------------------+");
  // console.//log("|            StoneDispatchToHandler              |");
  // console.//log("+------------------------------------------------+");
  // console.//log("strValue = ");
  // console.//log(strValue);
  let jsonValue: any = JSON.parse(strValue)
  return StoneDispatchJsonToHandler(jsonValue, handler);
}
