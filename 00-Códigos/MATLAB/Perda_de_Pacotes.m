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

% Filtrar os dados dentro do intervalo de datas
idx = T.created_at >= data_inicio & T.created_at <= data_fim;
datas_filtradas = T.created_at(idx);
fieldX_filtrado = T.field6(idx);  % Alterando para field6 (Taxa de Perda de Pacotes)

% Remover valores NaN e valores anômalos (ex: fora de uma faixa específica), mantendo valores até 100
mask = ~isnan(fieldX_filtrado) & fieldX_filtrado >= 0 & fieldX_filtrado <= 100;
datas_limpas = datas_filtradas(mask);
fieldX_limpas = fieldX_filtrado(mask);

% Parâmetros do filtro (tamanho da janela)
windowSize = 5;  % Ajuste conforme necessário

% 1. Filtro de Média Móvel
fieldX_media_movel = movmean(fieldX_limpas, windowSize);

% 2. Filtro Mediana
fieldX_mediana = medfilt1(fieldX_limpas, windowSize);

% Gerar os gráficos diretamente no layout atual (sem nova janela)

% Gráfico original
subplot(3,1,1);
plot(datas_limpas, fieldX_limpas, 'Color', [1, 0.5, 0]);  % Laranja
title('Dados Originais - Taxa de Perda de Pacotes (TPP)');
xlabel('Data');
ylabel('TPP (%)');
grid on;

% Gráfico com Filtro de Média Móvel
subplot(3,1,2);
plot(datas_limpas, fieldX_media_movel, 'Color', [0, 0.4470, 0.7410]);  % Azul
title('Filtro Média Móvel- Taxa de Perda de Pacotes (TPP)');
xlabel('Data');
ylabel('TPP (%)');
grid on;

% Gráfico com Filtro Mediana
subplot(3,1,3);
plot(datas_limpas, fieldX_mediana, 'Color', [0.4660, 0.6740, 0.1880]);  % Verde
title('Filtro Mediana - Taxa de Perda de Pacotes (TPP)');
xlabel('Data');
ylabel('TPP (%)');
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
