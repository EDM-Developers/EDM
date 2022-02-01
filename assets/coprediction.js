const update_centered_equations = function () {
  // Read in the manifold specifications from the sliders
  const numObs = parseInt(document.getElementById("numObs").value);
  const E = parseInt(document.getElementById("E").value);
  const tau = parseInt(document.getElementById("tau").value);
  const allowMissing = false;

  // Construct the manifold and targets
  const p = 1;
  const {manifold: M_x, targets: y_L_x} = manifold("x", numObs, E, tau, allowMissing, p);
  const {manifold: M_z, targets: y_P_z} = manifold("z", numObs, E, tau, allowMissing, p);

  const p_xmap = 0;
  const M_u = manifold("u", numObs, E, tau, allowMissing, p_xmap).manifold;
  const y_L_v = manifold("v", numObs, E, tau, allowMissing, p_xmap).targets;
  const {manifold: M_w, targets: y_P_w} = manifold("w", numObs, E, tau, allowMissing, p_xmap);

  // Save the result to the page
  let eqnsToTypeset = [];

  const equations = document.querySelectorAll(".dynamic-equation");
  equations.forEach((eqn) => {
    const prevRenderedEquation = eqn.dataset.renderedEquation;
    const renderedEquation = eqn.dataset.equation
      .replace(/\${M_x}/, latexify(M_x))
      .replace(/\${y_L_x}/, latexify(y_L_x))
      .replace(/\${M_z}/, latexify(M_z))
      .replace(/\${y_P_z}/, latexify(y_P_z))
      .replace(/\${M_u}/, latexify(M_u))
      .replace(/\${y_L_v}/, latexify(y_L_v))
      .replace(/\${M_w}/, latexify(M_w))
      .replace(/\${y_P_w}/, latexify(y_P_w));

    if (prevRenderedEquation != renderedEquation) {
      eqnsToTypeset.push(eqn);
      MathJax.typesetClear([eqn]);
      eqn.innerHTML = renderedEquation;
      eqn.dataset.renderedEquation = renderedEquation;
    }
  });

  MathJax.typesetPromise(eqnsToTypeset);
};

const sliders = document.querySelectorAll(".slider-container input");

sliders.forEach((slider) =>
  slider.addEventListener("input", () => update_centered_equations())
);

update_centered_equations();
