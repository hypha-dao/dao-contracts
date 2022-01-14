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
    // The testing framework always rounds on the last quarter of a second and doesn't have the Z
    // 22:47:12.867Z becomes 22:47:13
    const copy = new Date(date);
    if (copy.getMilliseconds() >= 750) {
        copy.setSeconds(copy.getSeconds() + 1);
    }

    copy.setMilliseconds(0);
    return copy.toISOString().split('.')[0] + '.000';
};

export const nextDay = (environment: DaoBlockchain, date: Date, offsetMinutes: number = 10) => {
  // Sets the time to the end of proposal
  const tomorrow = new Date();
  tomorrow.setDate(date.getDate() + 1);
  tomorrow.setMinutes(tomorrow.getMinutes() + offsetMinutes);
  environment.setCurrentTime(tomorrow);
  return tomorrow;
}

export const setDate = (environment: DaoBlockchain, date: Date, offsetMinutes: number = 10) => {
  const next = new Date(date);
  //next.setMinutes(next.getMinutes() + offsetMinutes);
  environment.setCurrentTime(next);
  environment.currentDate = next;
  return next;
}
