import { DaoBlockchain } from "../dao/DaoBlockchain";

export const toISOString = (date: Date) => {
    // The testing framework always set the mills to 000 and doesn't have the Z
    return date.toISOString().split('.')[0] + '.000';
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
  return next;
}