import * as VsolMessages from "./VsolMessages_generated";

function TEST_StoneGen_SerializeComplex() {
  let msg1_0 = new VsolMessages.Message1();
  msg1_0.a = 42;
  msg1_0.b = "Benjamin";
  msg1_0.c = VsolMessages.EnumMonth0.January;
  msg1_0.d = true;
  let msg1_1 = new VsolMessages.Message1();
  msg1_1.a = 43;
  msg1_1.b = "Sandrine";
  msg1_1.c = VsolMessages.EnumMonth0.March;
  msg1_0.d = false;
  let result1_0 = msg1_0.StoneSerialize();
  let resultStr1_0 = JSON.stringify(result1_0);
  let result1_1 = msg1_1.StoneSerialize();
  let resultStr1_1 = JSON.stringify(result1_1);
  // std::string toto;
  // std::vector<Message1> tata;
  // std::vector<std::string> tutu;
  // std::map<int32_t, std::string> titi;
  // std::map<int32_t, Message1> lulu;
  let msg2_0 = new VsolMessages.Message2();
  msg2_0.toto = "Prout zizi";
  msg2_0.tata.push(msg1_0);
  msg2_0.tata.push(msg1_1);
  msg2_0.tutu.push("Mercadet");
  msg2_0.tutu.push("Poisson");
  msg2_0.titi["44"] = "key 44";
  msg2_0.titi["45"] = "key 45";
  msg2_0.lulu["54"] = msg1_1;
  msg2_0.lulu["55"] = msg1_0;
  let result2 = msg2_0.StoneSerialize();
  let resultStr2 = JSON.stringify(result2);
  let refResult2 = `{
"type" : "VsolMessages.Message2",
"value" : 
{
  "lulu" : 
  {
    "54" : 
    {
      "a" : 43,
      "b" : "Sandrine",
      "c" : 2,
      "d" : true
    },
    "55" : 
    {
      "a" : 42,
      "b" : "Benjamin",
      "c" : 0,
      "d" : false
    }
  },
  "tata" : 
  [
    {
      "a" : 42,
      "b" : "Benjamin",
      "c" : 0,
      "d" : false
    },
    {
      "a" : 43,
      "b" : "Sandrine",
      "c" : 2,
      "d" : true
    }
  ],
  "titi" : 
  {
    "44" : "key 44",
    "45" : "key 45"
  },
  "toto" : "Prout zizi",
  "tutu" : 
  [
    "Mercadet",
    "Poisson"
  ]
}
}
`;
  let refResult2Obj = JSON.parse(refResult2);
  let resultStr2Obj = JSON.parse(resultStr2);
  if (false) {
    if (refResult2Obj !== resultStr2Obj) {
      console.log("Results are different!");
      console.log(`refResult2Obj['value']['lulu']['54'] = ${refResult2Obj['value']['lulu']['54']}`);
      console.log(`refResult2Obj['value']['lulu']['54']['a'] = ${refResult2Obj['value']['lulu']['54']['a']}`);
      console.log("************************************************************");
      console.log("**                  REFERENCE OBJ                         **");
      console.log("************************************************************");
      console.log(refResult2Obj);
      console.log("************************************************************");
      console.log("**                  ACTUAL OBJ                            **");
      console.log("************************************************************");
      console.log(resultStr2Obj);
      console.log("************************************************************");
      console.log("**                  REFERENCE                             **");
      console.log("************************************************************");
      console.log(refResult2);
      console.log("************************************************************");
      console.log("**                  ACTUAL                                **");
      console.log("************************************************************");
      console.log(resultStr2);
      throw new Error("Wrong serialization");
    }
  }
  let refResultValue = JSON.parse(resultStr2);
  console.log(refResultValue);
}
class MyDispatcher {
  message1: VsolMessages.Message1;
  message2: VsolMessages.Message2;

  HandleMessage1(value: VsolMessages.Message1) {
    this.message1 = value;
    return true;
  }
  HandleMessage2(value: VsolMessages.Message2) {
    this.message2 = value;
    return true;
  }
  HandleA(value) {
    return true;
  }
  HandleB(value) {
    return true;
  }
  HandleC(value) {
    return true;
  }
}
;
function TEST_StoneGen_DeserializeOkAndNok() {
  let serializedMessage = `{
"type" : "VsolMessages.Message2",
"value" : 
{
  "lulu" : 
  {
    "54" : 
    {
      "a" : 43,
      "b" : "Sandrine",
      "c" : 2,
      "d" : true
    },
    "55" : 
    {
      "a" : 42,
      "b" : "Benjamin",
      "c" : 0,
      "d" : false
    }
  },
  "tata" : 
  [
    {
      "a" : 42,
      "b" : "Benjamin",
      "c" : 0,
      "d" : false
    },
    {
      "a" : 43,
      "b" : "Sandrine",
      "c" : 2,
      "d" : true
    }
  ],
  "titi" : 
  {
    "44" : "key 44",
    "45" : "key 45"
  },
  "toto" : "Prout zizi",
  "tutu" : 
  [
    "Mercadet",
    "Poisson"
  ]
}
}`;
  let myDispatcher = new MyDispatcher();
  let ok = VsolMessages.StoneDispatchToHandler(serializedMessage, myDispatcher);
  if (!ok) {
    throw Error("Error when dispatching message!");
  }
  if (myDispatcher.message1 != undefined) {
    throw Error("(myDispatcher.Message1 != undefined)");
  }
  if (myDispatcher.message2 == undefined) {
    throw Error("(myDispatcher.Message2 == undefined)");
  }
  console.log("TEST_StoneGen_DeserializeOkAndNok: OK!");
}
function main() {
  console.log("Entering main()");
  TEST_StoneGen_SerializeComplex();
  TEST_StoneGen_DeserializeOkAndNok();
  return 0;
}
console.log(`Exit code is: ${main()}`);