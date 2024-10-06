clc;
clear all;

% Carregar o arquivo CSV como uma tabela
T = readtable('SIM.csv');

% Converter a coluna 'created_at' para datetime, incluindo o fuso horário
T.created_at = datetime(T.created_at, 'InputFormat', 'yyyy-MM-dd''T''HH:mm:ssXXX', 'TimeZone', 'America/Sao_Paulo');

% Se desejar remover o fuso horário após a conversão
T.created_at.TimeZone = '';

%% Defina aqui as datas de início e fim para filtrar os dados
data_inicio = datetime(2024, 9, 24);  % Data de início desejada
data_fim = datetime(2024, 9, 28);     % Data de fim desejada

% Criar uma linha de tempo completa entre as datas de início e fim
datas_completas = (data_inicio:data_fim)';

% Filtrar os dados do dia 24 de setembro
idx_24 = T.created_at >= datetime(2024, 9, 24, 0, 0, 0) & T.created_at < datetime(2024, 9, 25, 0, 0, 0);
datas_24 = T.created_at(idx_24);
fieldX_24 = T.field4(idx_24);  % Usar field4 para Índice UV

% Preencher com zeros para datas de 25 a 28 de setembro
fieldX_completas = zeros(length(datas_completas), 1);

% Preencher os dados reais do dia 24 de setembro
for i = 1:length(datas_completas)
    if datas_completas(i) == datetime(2024, 9, 24)
        fieldX_completas(i) = mean(fieldX_24);  % Usar a média dos valores do dia 24, caso haja mais de um
    end
end

% Parâmetros do filtro (tamanho da janela)
windowSize = 5;  % Ajuste conforme necessário

% 1. Filtro de Média Móvel
fieldX_media_movel = movmean(fieldX_completas, windowSize);

% 2. Filtro Mediana
fieldX_mediana = medfilt1(fieldX_completas, windowSize);

% Gerar os gráficos diretamente no layout atual (sem nova janela)

% Gráfico original
subplot(3,1,1);
plot(datas_completas, fieldX_completas, 'Color', [1, 0.5, 0]);  % Laranja
title('Dados Originais - Índice UV');
xlabel('Data');
ylabel('Índice UV');
ylim([0 3]); % Escala padronizada de 0 a 3
grid on;

% Gráfico com Filtro de Média Móvel
subplot(3,1,2);
plot(datas_completas, fieldX_media_movel, 'Color', [0, 0.4470, 0.7410]);  % Azul
title('Filtro Média Móvel - Índice UV');
xlabel('Data');
ylabel('Índice UV');
ylim([0 3]); % Escala padronizada de 0 a 3
grid on;

% Gráfico com Filtro Mediana
subplot(3,1,3);
plot(datas_completas, fieldX_mediana, 'Color', [0.4660, 0.6740, 0.1880]);  % Verde
title('Filtro Mediana - Índice UV');
xlabel('Data');
ylabel('Índice UV');
ylim([0 3]); % Escala padronizada de 0 a 3
grid on;

% Ajustar o eixo x para exibir datas em português
ax1 = subplot(3,1,1);
ax2 = subplot(3,1,2);
ax3 = subplot(3,1,3);

ax1.XAxis.TickLabelFormat = 'dd-MMM-yyyy';  % Ajusta o formato para dia-mês-ano
ax2.XAxis.TickLabelFormat = 'dd-MMM-yyyy';
ax3.XAxis.TickLabelFormat = 'dd-MMM-yyyy';

% Alterar os nomes dos meses para português
ax1.XTickLabel = strrep(ax1.XTickLabel, 'Oct', 'Out');
ax1.XTickLabel = strrep(ax1.XTickLabel, 'Sep', 'Set');
ax1.XTickLabel = strrep(ax1.XTickLabel, 'Jul', 'Jul');

ax2.XTickLabel = strrep(ax2.XTickLabel, 'Oct', 'Out');
ax2.XTickLabel = strrep(ax2.XTickLabel, 'Sep', 'Set');
ax2.XTickLabel = strrep(ax2.XTickLabel, 'Jul', 'Jul');

ax3.XTickLabel = strrep(ax3.XTickLabel, 'Oct', 'Out');
ax3.XTickLabel = strrep(ax3.XTickLabel, 'Sep', 'Set');
ax3.XTickLabel = strrep(ax3.XTickLabel, 'Jul', 'Jul');
