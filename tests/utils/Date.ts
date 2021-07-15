import { DaoBlockchain } from "../dao/DaoBlockchain";

export const toISOString = (date: Date) => {
    // The testing framework always rounds the  millis and doesn't have the Z
    // 22:47:12.867Z becomes 22:47:13
    const copy = new Date(date);
    copy.setSeconds(Math.round(parseFloat(`${copy.getSeconds()}.${copy.getMilliseconds()}`)));
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
  next.setMinutes(next.getMinutes() + offsetMinutes);
  environment.setCurrentTime(next);
  environment.currentDate = next;
  return next;
}