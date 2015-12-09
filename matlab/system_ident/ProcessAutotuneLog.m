function [] = ProcessAutotuneLog(fileStr, makeGraphs)
%/**
% ******************************************************************************
% * @file       ProcessAutotuneLog.m
% * @author     dRonin, http://dRonin.org/, Copyright (C) 2015
% * @addtogroup [Group]
% * @{
% * @addtogroup %CLASS%
% * @{
% * @brief [Brief]
% *****************************************************************************/
%/*
% * This program is free software; you can redistribute it and/or modify
% * it under the terms of the GNU General Public License as published by
% * the Free Software Foundation; either version 3 of the License, or
% * (at your option) any later version.
% *
% * This program is distributed in the hope that it will be useful, but
% * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
% * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
% * for more details.
% *
% * You should have received a copy of the GNU General Public License along
% * with this program; if not, write to the Free Software Foundation, Inc.,
% * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
% */

  close all;
  Roll = 1;
  Pitch = 2;
  Yaw = 3;
  
  %get data from log file
  LogConvert(fileStr, true);
  LogData = load([fileStr(1:end-3) 'mat']);
  
  if makeGraphs
    %make some pretty pictures
%    figure; plot(LogData.SystemIdent.NumAfPredicts(10:end-1), [LogData.SystemIdent.Noise(:,10:end-1)]);
%    legend('Roll', 'Pitch', 'Yaw');
%    title('Noise');
%    figure; plot(LogData.SystemIdent.NumAfPredicts(10:end-1), [LogData.SystemIdent.Bias(:,10:end-1)]);
%    legend('Roll', 'Pitch', 'Yaw');
%    title('Bias');
    figure; plot(LogData.SystemIdent.NumAfPredicts(10:end-1), [LogData.SystemIdent.Beta(:,10:end-1)]);
    legend('Roll', 'Pitch', 'Yaw');
    title('Beta');
    figure; plot(LogData.SystemIdent.NumAfPredicts(10:end-1), 1000*exp(LogData.SystemIdent.Tau(10:end-1)));
    title('Tau (ms)');
  end
  
  %default values
  ghf = 0.01;%m_autotune->rateNoise->value() / 1000.0;
  damp = 1.1;%m_autotune->rateDamp->value() / 100.0;
  
  numSIPoints = size(LogData.SystemIdent.NumAfPredicts,2);
  Rollki = zeros(1,numSIPoints);
  Rollkp = zeros(1,numSIPoints);
  Rollkd = zeros(1,numSIPoints);
  Pitchki = zeros(1,numSIPoints);
  Pitchkp = zeros(1,numSIPoints);
  Pitchkd = zeros(1,numSIPoints);
  DerivativeCutoff = zeros(1,numSIPoints);
  NaturalFrequency = zeros(1,numSIPoints);
  
  % get resulting PID values at each point in the logged data
  for ii=1:numSIPoints
    tau = exp(LogData.SystemIdent.Tau(ii)); %exp(systemIdentData.Tau(ii));
    beta_roll = LogData.SystemIdent.Beta(Roll,ii); %systemIdentData.Beta[SystemIdent::BETA_ROLL];
    beta_pitch = LogData.SystemIdent.Beta(Pitch,ii); %systemIdentData.Beta[SystemIdent::BETA_PITCH];

    wn = 1/tau;
    tau_d = 0;
    for jj=1:30
      tau_d_roll = (2*damp*tau*wn - 1)/(4*tau*damp*damp*wn*wn - 2*damp*wn - tau*wn*wn + exp(beta_roll)*ghf);
      tau_d_pitch = (2*damp*tau*wn - 1)/(4*tau*damp*damp*wn*wn - 2*damp*wn - tau*wn*wn + exp(beta_pitch)*ghf);

%      // Select the slowest filter property
      if (tau_d_roll > tau_d_pitch)
        tau_d = tau_d_roll;
      else
        tau_d = tau_d_pitch;
      end
        
      wn = (tau + tau_d) / (tau*tau_d) / (2 * damp + 2);
    end

%    // Set the real pole position. The first pole is quite slow, which
%    // prevents the integral being too snappy and driving too much
%    // overshoot.
    a = ((tau+tau_d) / tau / tau_d - 2 * damp * wn) / 20.0;
    b = ((tau+tau_d) / tau / tau_d - 2 * damp * wn - a);

%    // Calculate the gain for the outer loop by approximating the
%    // inner loop as a single order lpf. Set the outer loop to be
%    // critically damped;
    zeta_o = 1.3;
    kp_o(ii) = 1 / 4.0 / (zeta_o * zeta_o) / (1/wn);

    beta = exp(LogData.SystemIdent.Beta(Roll, ii));
    Rollki(ii) = a * b * wn * wn * tau * tau_d / beta;
    Rollkp(ii) = (tau * tau_d * ((a+b)*wn*wn + 2*a*b*damp*wn) / beta) - (Rollki(ii)*tau_d);
    Rollkd(ii) = (tau * tau_d * (a*b + wn*wn + (a+b)*2*damp*wn) - 1) / beta - (Rollkp(ii) * tau_d);

    beta = exp(LogData.SystemIdent.Beta(Pitch, ii));
    Pitchki(ii) = a * b * wn * wn * tau * tau_d / beta;
    Pitchkp(ii) = (tau * tau_d * ((a+b)*wn*wn + 2*a*b*damp*wn) / beta) - (Pitchki(ii)*tau_d);
    Pitchkd(ii) = (tau * tau_d * (a*b + wn*wn + (a+b)*2*damp*wn) - 1) / beta - (Pitchkp(ii) * tau_d);

    DerivativeCutoff(ii) = 1 / (2*pi*tau_d);
    NaturalFrequency(ii) = wn / 2 / pi;
  end
  
  if makeGraphs
    %make more pretty pictures
%    figure; plot(LogData.SystemIdent.NumAfPredicts(1:end-1), DerivativeCutoff(1:end-1));
%    title('DerivativeCutoff');
    figure; plot(LogData.SystemIdent.NumAfPredicts(1:end-1), NaturalFrequency(1:end-1));
    title('NaturalFrequency');
    figure; plot(LogData.SystemIdent.NumAfPredicts(1:end-1), kp_o(1:end-1));
    title('Outer Kp');
    figure; plot(LogData.SystemIdent.NumAfPredicts(1:end-1), [Rollki(1:end-1); Rollkp(1:end-1); Rollkd(1:end-1)]');
    title('Roll PID Values');
    legend('Rollki', 'Rollkp', 'Rollkd');
    figure; plot(LogData.SystemIdent.NumAfPredicts(1:end-1), [Pitchki(1:end-1); Pitchkp(1:end-1); Pitchkd(1:end-1)]');
    title('Pitch PID Values');
    legend('Pitchki', 'Pitchkp', 'Pitchkd');
  end
  
  % Covariance trace figure of merrit
  traceFOM = sum(LogData.SystemIdent.CovarianceMatrix,1);
  if makeGraphs
    figure; plot(LogData.SystemIdent.NumAfPredicts(10:end-1), traceFOM(10:end-1));
    title('tr(P)');
  end
  
  % find the best trace in the second half of the tuning run, exclude any data points where NumAfPredicts is zero (invalid data)
  [convValue convPoint] = min(traceFOM(end/2:end) + 1000*(!LogData.SystemIdent.NumAfPredicts(end/2:end)));
  convPoint = round(convPoint + size(traceFOM,2)/2 - 1);
  
  printf('\nMinimum Tr(P) value in last half: %f\n', convValue);
  printf('Minimum Tr(P) location in last half: %f\n', convPoint);
  printf('Minimum Tr(P) NumAfPredicts in last half: %f\n', LogData.SystemIdent.NumAfPredicts(convPoint));
  
  % print data and converged PID values
  printf('Achieved Tau (ms): %f\n', 1000*exp(LogData.SystemIdent.Tau(convPoint)));
  
  printf('\nOptimal Roll Kp: %f\n', Rollkp(convPoint));
  printf('Optimal Roll Ki: %f\n', Rollki(convPoint));
  printf('Optimal Roll Kd: %f\n', Rollkd(convPoint));
  
  printf('\nOptimal Pitch Kp: %f\n', Pitchkp(convPoint));
  printf('Optimal Pitch Ki: %f\n', Pitchki(convPoint));
  printf('Optimal Pitch Kd: %f\n', Pitchkd(convPoint));
  
  printf('\nOuter Kp: %f\n', kp_o(convPoint));
  