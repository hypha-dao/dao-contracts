import { DaoBlockchain } from "../dao/DaoBlockchain";

export const toUTC = (date: Date) => {
  let utc = Date.UTC(date.getUTCFullYear(), date.getUTCMonth(), date.getUTCDate(),
                     date.getUTCHours(), date.getUTCMinutes(), date.getUTCSeconds());
 return new Date(utc);
}

export const fromUTC = (date: Date) => {
  let local = new Date();
  local.setTime(date.getTime()-local.getTimezoneOffset()*60000);
  return local;
}

export const toISOString = (date: Date) => {
    // The testing framework truncates the  millis and doesn't have the Z
    // https://github.com/EOSIO/eosjs2/blob/723b14fe384b1c8ca4f1a7437a3d3ff2087a28ba/src/eosjs-serialize.ts#L556
    const value = Math.round(date.valueOf() / 500);
    const roundedDate = new Date(value);
    return roundedDate.toISOString().split('.')[0] + '.000';
};

export const nextDay = (environment: DaoBlockchain, date: Date, offsetMinutes: number = 10) => {
  // Sets the time to the end of proposal
  const tomorrow = new Date();
  tomorrow.setMilliseconds(0);
  tomorrow.setDate(date.getDate() + 1);
  tomorrow.setMinutes(tomorrow.getMinutes() + offsetMinutes);
  environment.setCurrentTime(tomorrow);
  return tomorrow;
}

export const setDate = (environment: DaoBlockchain, date: Date, offsetMinutes: number = 10) => {
  const next = new Date(date);
  next.setMilliseconds(0);
  next.setMinutes(next.getMinutes() + offsetMinutes);
  environment.setCurrentTime(next);
  environment.currentDate = next;
  return next;
}
