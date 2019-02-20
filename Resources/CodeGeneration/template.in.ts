/*
         1         2         3         4         5         6         7
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*/

namespace {{rootName}}
{
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
    if ( (!('type' in value)) || (!isString(value)) )
    {
      throw new Error(
        "Cannot deserialize value ('type' key invalid)");
    }
  }

// end of generic methods
{% for enum in enums%}
  export enum {{enum['name']}} {
    {% for key in enumDict.keys()%}
    {{key}},
    {%endfor%}
  };
{%endfor%}

  export class Message1 {
    a: number;
    b: string;
    c: EnumMonth0;
    d: boolean;
    public StoneSerialize(): string {
      let container: object = {};
      container['type'] = 'VsolStuff.Message1';
      container['value'] = this;
      return JSON.stringify(container);
    }
  };

  export class Message2 {
    constructor()
    {
      this.tata = new Array<Message1>();
      this.tutu = new Array<string>();
      this.titi = new Map<string, string>();
      this.lulu = new Map<string, Message1>();  
    }
    toto: string;
    tata: Message1[];
    tutu: string[];
    titi: Map<string, string>;
    lulu: Map<string, Message1>;

    public StoneSerialize(): string {
      let container: object = {};
      container['type'] = 'VsolStuff.Message2';
      container['value'] = this;
      return JSON.stringify(container);
    }
    public static StoneDeserialize(valueStr: string) : Message2
    {
      let value: any = JSON.parse(valueStr);
      StoneCheckSerializedValueType(value, "VsolStuff.Message2");
      let result: Message2 = value['value'] as Message2;
      return result;
    }
    };

  export interface IDispatcher
  {
    HandleMessage1(value: Message1): boolean;
    HandleMessage2(value: Message2): boolean;
  };

  /** Service function for StoneDispatchToHandler */
  export function StoneDispatchJsonToHandler(
    jsonValueStr: string, dispatcher: IDispatcher): boolean
  {
    let jsonValue: any = JSON.parse(jsonValueStr);
    StoneCheckSerializedValueTypeGeneric(jsonValue);
    let type: string = jsonValue["type"];
    if (type == "")
    {
      // this should never ever happen
      throw new Error("Caught empty type while dispatching");
    }
    else if (type == "VsolStuff.Message1")
    {
      let value = jsonValue["value"] as Message1;
      return dispatcher.HandleMessage1(value);
    }
    else if (type == "VsolStuff.Message2")
    {
      let value = jsonValue["value"] as Message2;
      return dispatcher.HandleMessage2(value);
    }
    else
    {
      return false;
    }
  }

  /** Takes a serialized type and passes this to the dispatcher */
  export function StoneDispatchToHandler(
    strValue: string, dispatcher: IDispatcher): boolean
  {
    let jsonValue: any = JSON.parse(strValue)
    return StoneDispatchJsonToHandler(jsonValue, dispatcher);
  }
}
