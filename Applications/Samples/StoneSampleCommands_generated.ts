/*
         1         2         3         4         5         6         7
12345678901234567890123456789012345678901234567890123456789012345678901234567890

Generated on 2019-03-15 10:00:48.763392 by stonegentool

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

export enum Tool {
  LineMeasure = "LineMeasure",
  CircleMeasure = "CircleMeasure",
  Crop = "Crop",
  Windowing = "Windowing",
  Zoom = "Zoom",
  Pan = "Pan",
  Move = "Move",
  Rotate = "Rotate",
  Resize = "Resize",
  Mask = "Mask"
};

export function Tool_FromString(strValue:string) : Tool
{
  if( strValue == "LineMeasure" )
  {
    return Tool.LineMeasure;
  }
  if( strValue == "CircleMeasure" )
  {
    return Tool.CircleMeasure;
  }
  if( strValue == "Crop" )
  {
    return Tool.Crop;
  }
  if( strValue == "Windowing" )
  {
    return Tool.Windowing;
  }
  if( strValue == "Zoom" )
  {
    return Tool.Zoom;
  }
  if( strValue == "Pan" )
  {
    return Tool.Pan;
  }
  if( strValue == "Move" )
  {
    return Tool.Move;
  }
  if( strValue == "Rotate" )
  {
    return Tool.Rotate;
  }
  if( strValue == "Resize" )
  {
    return Tool.Resize;
  }
  if( strValue == "Mask" )
  {
    return Tool.Mask;
  }

  let msg : string =  `String ${strValue} cannot be converted to Tool. Possible values are: LineMeasure, CircleMeasure, Crop, Windowing, Zoom, Pan, Move, Rotate, Resize, Mask`;
  throw new Error(msg);
}

export function Tool_ToString(value:Tool) : string
{
  if( value == Tool.LineMeasure )
  {
    return "LineMeasure";
  }
  if( value == Tool.CircleMeasure )
  {
    return "CircleMeasure";
  }
  if( value == Tool.Crop )
  {
    return "Crop";
  }
  if( value == Tool.Windowing )
  {
    return "Windowing";
  }
  if( value == Tool.Zoom )
  {
    return "Zoom";
  }
  if( value == Tool.Pan )
  {
    return "Pan";
  }
  if( value == Tool.Move )
  {
    return "Move";
  }
  if( value == Tool.Rotate )
  {
    return "Rotate";
  }
  if( value == Tool.Resize )
  {
    return "Resize";
  }
  if( value == Tool.Mask )
  {
    return "Mask";
  }

  let msg : string = `Value ${value} cannot be converted to Tool. Possible values are: `;
  {
    let _LineMeasure_enumValue : string = Tool.LineMeasure; // enums are strings in stonecodegen, so this will work.
    let msg_LineMeasure : string = `LineMeasure (${_LineMeasure_enumValue}), `;
    msg = msg + msg_LineMeasure;
  }
  {
    let _CircleMeasure_enumValue : string = Tool.CircleMeasure; // enums are strings in stonecodegen, so this will work.
    let msg_CircleMeasure : string = `CircleMeasure (${_CircleMeasure_enumValue}), `;
    msg = msg + msg_CircleMeasure;
  }
  {
    let _Crop_enumValue : string = Tool.Crop; // enums are strings in stonecodegen, so this will work.
    let msg_Crop : string = `Crop (${_Crop_enumValue}), `;
    msg = msg + msg_Crop;
  }
  {
    let _Windowing_enumValue : string = Tool.Windowing; // enums are strings in stonecodegen, so this will work.
    let msg_Windowing : string = `Windowing (${_Windowing_enumValue}), `;
    msg = msg + msg_Windowing;
  }
  {
    let _Zoom_enumValue : string = Tool.Zoom; // enums are strings in stonecodegen, so this will work.
    let msg_Zoom : string = `Zoom (${_Zoom_enumValue}), `;
    msg = msg + msg_Zoom;
  }
  {
    let _Pan_enumValue : string = Tool.Pan; // enums are strings in stonecodegen, so this will work.
    let msg_Pan : string = `Pan (${_Pan_enumValue}), `;
    msg = msg + msg_Pan;
  }
  {
    let _Move_enumValue : string = Tool.Move; // enums are strings in stonecodegen, so this will work.
    let msg_Move : string = `Move (${_Move_enumValue}), `;
    msg = msg + msg_Move;
  }
  {
    let _Rotate_enumValue : string = Tool.Rotate; // enums are strings in stonecodegen, so this will work.
    let msg_Rotate : string = `Rotate (${_Rotate_enumValue}), `;
    msg = msg + msg_Rotate;
  }
  {
    let _Resize_enumValue : string = Tool.Resize; // enums are strings in stonecodegen, so this will work.
    let msg_Resize : string = `Resize (${_Resize_enumValue}), `;
    msg = msg + msg_Resize;
  }
  {
    let _Mask_enumValue : string = Tool.Mask; // enums are strings in stonecodegen, so this will work.
    let msg_Mask : string = `Mask (${_Mask_enumValue})`;
    msg = msg + msg_Mask;
  }
  throw new Error(msg);
}

export enum ActionType {
  UndoCrop = "UndoCrop",
  Rotate = "Rotate",
  Invert = "Invert"
};

export function ActionType_FromString(strValue:string) : ActionType
{
  if( strValue == "UndoCrop" )
  {
    return ActionType.UndoCrop;
  }
  if( strValue == "Rotate" )
  {
    return ActionType.Rotate;
  }
  if( strValue == "Invert" )
  {
    return ActionType.Invert;
  }

  let msg : string =  `String ${strValue} cannot be converted to ActionType. Possible values are: UndoCrop, Rotate, Invert`;
  throw new Error(msg);
}

export function ActionType_ToString(value:ActionType) : string
{
  if( value == ActionType.UndoCrop )
  {
    return "UndoCrop";
  }
  if( value == ActionType.Rotate )
  {
    return "Rotate";
  }
  if( value == ActionType.Invert )
  {
    return "Invert";
  }

  let msg : string = `Value ${value} cannot be converted to ActionType. Possible values are: `;
  {
    let _UndoCrop_enumValue : string = ActionType.UndoCrop; // enums are strings in stonecodegen, so this will work.
    let msg_UndoCrop : string = `UndoCrop (${_UndoCrop_enumValue}), `;
    msg = msg + msg_UndoCrop;
  }
  {
    let _Rotate_enumValue : string = ActionType.Rotate; // enums are strings in stonecodegen, so this will work.
    let msg_Rotate : string = `Rotate (${_Rotate_enumValue}), `;
    msg = msg + msg_Rotate;
  }
  {
    let _Invert_enumValue : string = ActionType.Invert; // enums are strings in stonecodegen, so this will work.
    let msg_Invert : string = `Invert (${_Invert_enumValue})`;
    msg = msg + msg_Invert;
  }
  throw new Error(msg);
}



export class SelectTool {
  tool:Tool;

  constructor() {
  }

  public StoneSerialize(): string {
    let container: object = {};
    container['type'] = 'StoneSampleCommands.SelectTool';
    container['value'] = this;
    return JSON.stringify(container);
  }

  public static StoneDeserialize(valueStr: string) : SelectTool
  {
    let value: any = JSON.parse(valueStr);
    StoneCheckSerializedValueType(value, 'StoneSampleCommands.SelectTool');
    let result: SelectTool = value['value'] as SelectTool;
    return result;
  }
}
export class Action {
  type:ActionType;

  constructor() {
  }

  public StoneSerialize(): string {
    let container: object = {};
    container['type'] = 'StoneSampleCommands.Action';
    container['value'] = this;
    return JSON.stringify(container);
  }

  public static StoneDeserialize(valueStr: string) : Action
  {
    let value: any = JSON.parse(valueStr);
    StoneCheckSerializedValueType(value, 'StoneSampleCommands.Action');
    let result: Action = value['value'] as Action;
    return result;
  }
}

export interface IHandler {
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