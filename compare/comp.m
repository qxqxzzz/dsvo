%% load data
close all
clear
dir = '/home/jiawei/Dropbox/mh_results/mh_01';
% dir = '~/.ros';
gt_a = load(strcat(dir,'/truth.txt'));
vo = load(strcat(dir,'/vo.txt'));

% get overlap of gt with vo
j = 1; % index of gt_a
for i=1:size(vo,1)
    while(j<=size(gt_a,1) && vo(i,1) > gt_a(j,1))
        j = j+1;
    end
    if(j>size(gt_a,1))
        break;
    else
        gt(i,1) = vo(i,1);
        inc = (gt_a(j,2:4) - gt_a(j-1,2:4))/(gt_a(j,1) - gt_a(j-1,1));
        gt(i,2:4) = gt_a(j-1,2:4) + (gt(i,1) - gt_a(j-1,1))*inc;
    end
end

vo = vo(1:size(gt,1), :);

% use range of idx
idx = 1:size(gt,1);
% idx = [size(gt,1):-1:size(gt,1)-200];
gt = gt(idx, :);
vo = vo(idx, :);

%% align two set of points
sz = size(gt,1);
% sz = 20;
A = zeros(3*sz, 12);
b = zeros(3*sz, 1);
% for i=1:size(gt,1)
for i=1:sz
    A(3*(i-1)+1, 1:3) = vo(i,2:4);
    A(3*(i-1)+2, 4:6) = vo(i,2:4);
    A(3*(i-1)+3, 7:9) = vo(i,2:4);
    A(3*(i-1)+1:3*(i-1)+3, 10:12) = eye(3);
    
    b(3*(i-1)+1:3*(i-1)+3, 1) = gt(i,2:4)';
end
x = A\b;
R = [x(1:3,1)'; x(4:6,1)'; x(7:9,1)'];
t = x(10:12,1);
[U,S,V] = svd(R);
R = U*V';
t = t / S(1,1);

for i=1:size(vo,1)
    p = R*vo(i,2:4)' + t;
    vo(i,2:4) = p';
end

%% plot results
step = 10;

% calculate vo velocity
for i=1+step:size(vo,1)
    vo(i, 5:7) = (vo(i, 2:4) - vo(i-step, 2:4)) / (vo(i, 1) - vo(i-step, 1));
end
vo = vo(1+step:end, :);

% calculate gt velocity
for i=1+step:size(gt,1)
    gt(i, 5:7) = (gt(i, 2:4) - gt(i-step, 2:4)) / (gt(i, 1) - gt(i-step, 1));
end
gt = gt(1+step:end, :);
 
% time axis
gtt = gt(:,1)-gt(1,1);
vot = vo(:,1)-vo(1,1);

% position
gtp = gt(:,2:4);
vop = vo(:,2:4);
figure('Name','Trajectory')
plot3(gtp(:,1), gtp(:,2), gtp(:,3), 'g-')
hold on
plot3(vop(:,1), vop(:,2), vop(:,3), 'r-')
legend('Truth', 'Estimated');
title('Trajectory')

% velocity scale
gtvn = vecnorm(gt(:,5:7)')';
vovn = vecnorm(vo(:,5:7)')';
diffn = gtvn - vovn;
rmsen = sqrt(diffn' * diffn / size(diffn,1));
medn = median(abs(diffn));
pern = abs(diffn) ./ gtvn;
pern = sum(pern) / length(pern);
fprintf('RMSE offset of velocity = %f\n', rmsen);
fprintf('Median offset of velocty = %f\n', medn);
fprintf('Average offset percentage of velocty = %f\n', pern);
figure('Name','Velocity Scale')
plot(gtt, gtvn, 'go-');
hold on
plot(vot, vovn, 'r*-');
legend('Truth', 'Estimated');
title('Velocity Scale');

% velocity orientation
gtvd = gt(:,5:7) ./ gtvn;
vovd = vo(:,5:7) ./ vovn;
gt_vel_ori = [gtt, gtt, gtt];
vo_vel_ori = [vot, vot, vot];
for i=2:size(vo,1)
    gt_vel_ori(i,:) = acos( gtvd(i,:))/3.14159*180;
    vo_vel_ori(i,:) = acos( vovd(i,:))/3.14159*180;
end
figure('Name','Velocity Direction');
subplot(3,1,1);
plot(gtt, gt_vel_ori(:,1), 'go-');
hold on
plot(vot, vo_vel_ori(:,1), 'r*-');
title('x direction');
legend('Truth', 'Estimated');
subplot(3,1,2);
plot(gtt, gt_vel_ori(:,2), 'go-');
hold on
plot(vot, vo_vel_ori(:,2), 'r*-');
title('y direction');
subplot(3,1,3);
plot(gtt, gt_vel_ori(:,3), 'go-');
hold on
plot(vot, vo_vel_ori(:,3), 'r*-');
title('z direction');